#!/usr/bin/env python3
"""
Download all amenity icons referenced in categories.html into a local folder.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
import re
from typing import Dict, Tuple, Optional
from urllib.parse import urljoin, urlparse, urlunparse

import requests
from bs4 import BeautifulSoup

from gen import make_string_name, extract_categories


DEFAULT_BASE_URL = "https://wiki.openstreetmap.org"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "html_path",
        nargs="?",
        default="categories.html",
        help="Path to the HTML file containing the icon table (default: categories.html).",
    )
    parser.add_argument(
        "--output",
        "-o",
        default="icons",
        help="Directory where icons will be written (default: icons).",
    )
    parser.add_argument(
        "--base-url",
        default=DEFAULT_BASE_URL,
        help=f"Base URL for resolving relative icon paths (default: {DEFAULT_BASE_URL}).",
    )
    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="Re-download icons even if they already exist locally.",
    )
    return parser.parse_args()


def pick_image_url(img) -> str | None:
    """Select the highest resolution URL from an <img> tag."""
    srcset = img.get("srcset")
    candidates: list[Tuple[float, str]] = []

    if srcset:
        for part in srcset.split(","):
            tokens = part.strip().split()
            if not tokens:
                continue
            url = tokens[0]
            scale = tokens[1] if len(tokens) > 1 else "1x"
            weight = 1.0
            if scale.endswith("x"):
                try:
                    weight = float(scale[:-1])
                except ValueError:
                    weight = 1.0
            elif scale.endswith("w"):
                try:
                    weight = float(scale[:-1])
                except ValueError:
                    weight = 1.0
            candidates.append((weight, url))

    if candidates:
        candidates.sort(key=lambda item: item[0], reverse=True)
        return candidates[0][1]

    return img.get("src")


def derive_svg_url(candidate_url: str, base_url: str, name_source: str) -> Optional[str]:
    """Convert a candidate (often PNG) URL to the underlying SVG asset."""
    if not candidate_url:
        return None

    resolved = urljoin(base_url, candidate_url)
    parsed = urlparse(resolved)
    path = parsed.path

    if path.endswith(".svg"):
        return urlunparse(parsed._replace(query="", fragment=""))

    if path.endswith(".svg.png"):
        path = path[:-4]

    if "/thumb/" in path:
        parts = path.split("/")
        try:
            idx = parts.index("thumb")
        except ValueError:
            pass
        else:
            head = parts[:idx]
            tail = parts[idx + 1 : -1]
            parts = head + tail
            if not parts or parts[0] != "":
                parts = [""] + parts
            path = "/".join(parts)

    if path.endswith(".svg"):
        return urlunparse(parsed._replace(path=path, query="", fragment=""))

    file_name = ""
    if name_source:
        file_name = name_source.split("/")[-1]
        file_name = file_name.split("?", 1)[0]
        if file_name.startswith("File:"):
            file_name = file_name[len("File:") :]

    if file_name.endswith(".svg"):
        special = f"/wiki/Special:FilePath/{file_name}"
        return urljoin(base_url, special)

    return None


def cleanup_basename(name: str) -> str:
    """Strip size suffixes and redundant `_svg` markers from filenames."""
    cleaned = name
    cleaned = re.sub(r'(_svg)+$', '', cleaned)
    cleaned = cleaned.replace('_svg_', '_')
    cleaned = re.sub(r'_svg$', '', cleaned)
    cleaned = re.sub(r'_(\d+)px$', r'_\1', cleaned)
    cleaned = cleaned.rstrip('_')
    return cleaned or name


def collect_icons(html: str, base_url: str) -> Dict[str, str]:
    """Parse HTML and collect a mapping of icon basename -> absolute URL."""
    soup = BeautifulSoup(html, "html.parser")
    icons: Dict[str, str] = {}
    name_map: Dict[str, str] = {}
    for entry in extract_categories(html):
        base = (entry["icon"] or entry["description"]).strip()
        slug = make_string_name(entry["icon"] or entry["description"])
        if base not in name_map:
            name_map[base] = slug
        if entry.get("icon"):
            icon = entry["icon"].strip()
            for ext in ("", ".svg", ".png", ".SVG", ".PNG"):
                variant = f"{icon}{ext}"
                if variant and variant not in name_map:
                    name_map[variant] = slug

    for row in soup.find_all("tr"):
        cells = row.find_all("td")
        if len(cells) < 1:
            continue
        icon_cell = cells[0]
        img = icon_cell.find("img")
        if not img:
            continue

        candidate_url = pick_image_url(img)

        anchor = icon_cell.find("a", class_="mw-file-description")
        if anchor and anchor.has_attr("href"):
            name_source = anchor["href"]
        elif img.has_attr("src"):
            name_source = img["src"]
        else:
            name_source = ""

        svg_url = derive_svg_url(candidate_url or "", base_url, name_source)
        if not svg_url:
            continue

        parsed_svg = urlparse(svg_url)

        key = anchor["href"] if anchor and anchor.has_attr("href") else name_source
        key = key.split("/")[-1] if key else ""
        if key.startswith("File:"):
            key = key[len("File:") :]
        base_name = name_map.get(
            key, make_string_name(key) or make_string_name(Path(parsed_svg.path).stem or "icon")
        )
        base_name = cleanup_basename(base_name)

        dest_name = f"{base_name}.svg"

        # Keep the first occurrence to avoid needless duplicates.
        icons.setdefault(dest_name, svg_url)

    return icons


def download_icon(url: str, destination: Path) -> None:
    response = requests.get(url, timeout=30)
    response.raise_for_status()
    destination.write_bytes(response.content)


def main() -> int:
    args = parse_args()
    html_path = Path(args.html_path)

    if not html_path.is_file():
        print(f"HTML file not found: {html_path}", file=sys.stderr)
        return 1

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    html_content = html_path.read_text(encoding="utf-8")
    icons = collect_icons(html_content, args.base_url)

    if not icons:
        print("No icons found in the provided HTML.", file=sys.stderr)
        return 1

    downloaded = 0
    skipped = 0
    for filename, url in sorted(icons.items()):
        target = output_dir / filename
        if target.exists() and not args.overwrite:
            skipped += 1
            continue
        try:
            download_icon(url, target)
            downloaded += 1
        except requests.RequestException as exc:
            print(f"Failed to download {url}: {exc}", file=sys.stderr)

    print(f"Icons found: {len(icons)}")
    print(f"Downloaded: {downloaded}")
    print(f"Skipped (already present): {skipped}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
