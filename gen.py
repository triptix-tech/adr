#!/usr/bin/env python3
"""
Generate C++ header for parsing OSM amenity categories from HTML table.
"""

from bs4 import BeautifulSoup, NavigableString
import re
from jinja2 import Template
from typing import List, Dict, Tuple, Optional

def sanitize_enum_name(name: str) -> str:
    """Convert category name to valid C++ enum name."""
    # Remove special characters and convert to snake_case
    name = re.sub(r'[^\w\s-]', '', name)
    name = re.sub(r'[-\s]+', '_', name)
    return 'k' + ''.join(word.capitalize() for word in name.split('_'))

def make_string_name(name: str) -> str:
    """Derive lower-case identifier for enum string lookup."""
    slug = name.lower()
    slug = re.sub(r'[^a-z0-9]+', '_', slug)
    slug = re.sub(r'_+', '_', slug).strip('_')
    return slug or name.lower()

def normalize_icon_name(raw: str) -> str:
    """Normalize wiki icon reference to a stable identifier (no prefix/extension/size)."""
    if not raw:
        return ''

    name = raw.split('/')[-1]
    name = name.split('?', 1)[0]

    name = re.sub(r'^\d+px-', '', name)
    if name.startswith('File:'):
        name = name[len('File:'):]

    # Strip known extensions repeatedly (e.g., .svg.png)
    while True:
        stripped = re.sub(r'\.(svg|png|gif)$', '', name, flags=re.IGNORECASE)
        if stripped == name:
            break
        name = stripped

    # Drop trailing size suffixes like -14, _14, or -14px
    while True:
        stripped = re.sub(r'[-_\.\s](\d+)(px)?$', '', name)
        if stripped == name:
            break
        name = stripped

    return name

def parse_tag_combinations(tag_cell) -> List[List[str]]:
    """Capture tag combinations preserving '+' as AND and '/' or <br> as OR."""
    combos: List[List[str]] = []
    current_combo: List[str] = []
    pending_sep: Optional[str] = None

    def flush_current() -> None:
        nonlocal current_combo
        if current_combo:
            combos.append(current_combo)
            current_combo = []

    for node in tag_cell.children:
        if isinstance(node, NavigableString):
            text = str(node)
            if not text.strip():
                continue
            if '/' in text:
                pending_sep = '/'
            elif '+' in text:
                pending_sep = '+'
            continue

        if getattr(node, "name", None) == "br":
            flush_current()
            pending_sep = None
            continue

        if node.name == "code":
            tag_text = node.get_text(strip=True)
            if not tag_text or '=' not in tag_text:
                continue
            if pending_sep == '/':
                flush_current()
            current_combo.append(tag_text)
            pending_sep = None
            continue

        # Some cells wrap <code> inside spans; attempt to drill down once.
        inner_code = node.find('code')
        if inner_code:
            tag_text = inner_code.get_text(strip=True)
            if tag_text and '=' in tag_text:
                if pending_sep == '/':
                    flush_current()
                current_combo.append(tag_text)
                pending_sep = None

    flush_current()
    return combos

def extract_categories(html_content: str) -> List[Dict]:
    """Extract one category entry per SVG/icon row from HTML table."""
    soup = BeautifulSoup(html_content, 'html.parser')
    table = soup.find('table', class_='wikitable')
    if not table:
        raise ValueError("Could not find wikitable")

    entries = []
    for row in table.find_all('tr'):
        cells = row.find_all('td')
        if len(cells) >= 3:
            # Extract icon filename
            icon_cell = cells[0]
            icon_link = icon_cell.find('a', class_='mw-file-description')
            icon_src = ''
            if icon_link and icon_link.has_attr('href'):
                icon_src = icon_link['href']
            else:
                img = icon_cell.find('img')
                if img and 'src' in img.attrs:
                    icon_src = img['src']
            icon_name = normalize_icon_name(icon_src)

            # Extract description
            description = cells[1].get_text(strip=True)

            # Extract tags
            tag_cell = cells[2]
            tag_combos = parse_tag_combinations(tag_cell)
            flat_tags = [tag for combo in tag_combos for tag in combo]

            if flat_tags:
                entries.append({
                    'icon': icon_name,
                    'description': description,
                    'tag_combos': tag_combos
                })
    return entries

def parse_tag(tag_str: str) -> Tuple[str, str]:
    """Parse OSM tag string into key and value."""
    if '=' not in tag_str:
        return None, None
    key, value = tag_str.split('=', 1)
    return key.strip(), value.strip()

def generate_cpp_header(entries: List[Dict]) -> str:
    """Generate C++ header file using Jinja2 template, one rule per SVG/icon row."""
    template = Template(
        '''// WARNING: This file is auto-generated. Do not edit manually.        

#pragma once

#include "cista/hash.h"
#include "osmium/osm/object.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace adr {

enum class amenity_category : std::uint16_t {
  kNone = 0,
{% for entry in entries %}
  {{ entry.enum_name }},
{% endfor %}
  kExtra
};

constexpr std::array<char const*, {{ entries|length + 2 }}> amenity_category_names = {
  "none",
{% for entry in entries %}
  "{{ entry.string_name }}",
{% endfor %}
  "extra"
};

inline constexpr char const* to_str(amenity_category const cat) {
  return amenity_category_names[static_cast<std::size_t>(cat)];
}

struct amenity_tags {
  explicit amenity_tags(osmium::TagList const& tags) {
    using namespace std::string_view_literals;
    // Single pass over all tags
    for (auto const& t : tags) {
      switch (cista::hash(std::string_view{t.key()})) {
{% for key, var_name in tag_keys.items() %}
        case cista::hash("{{ key }}"): {{ var_name }}_ = t.value(); break;
{% endfor %}
        default: break;
      }
    }
  }

  amenity_category get_category() const {
    using namespace std::string_view_literals;
{% for entry in entries %}
    // {{ entry.name_source }}
{% for condition in entry.conditions %}
    {{ condition }}
{% endfor %}
{% endfor %}
    return amenity_category::kNone;
  }

private:
{% for var_decl in member_vars %}
  {{ var_decl }};
{% endfor %}
};

}  // namespace osr
''',
        trim_blocks=True,
        lstrip_blocks=True,
    )

    # Collect all unique keys and create member variables
    tag_keys = {}
    member_vars = []
    all_keys = set()
    for entry in entries:
        for combo in entry['tag_combos']:
            for tag_str in combo:
                key, _ = parse_tag(tag_str)
                if key:
                    all_keys.add(key)
    for key in sorted(all_keys):
        var_name = key.replace(':', '_').replace('-', '_')
        tag_keys[key] = var_name
        member_vars.append(f"std::string_view {var_name}_")

    processed_entries = []
    seen_enum_names = set()
    for entry in entries:
        name_source = entry['icon'] or entry['description']
        if not name_source:
            name_source = entry['description']
        entry['name_source'] = name_source
        entry['enum_name'] = sanitize_enum_name(name_source)
        entry['string_name'] = make_string_name(name_source)
        if entry['enum_name'] in seen_enum_names:
            continue
        seen_enum_names.add(entry['enum_name'])
        processed_entries.append(entry)
    entries = processed_entries

    # Build conditions for each entry
    for entry in entries:
        conditions: List[str] = []
        for combo in entry['tag_combos']:
            combo_conditions: List[str] = []
            for tag_str in combo:
                parts = [p.strip() for p in tag_str.split('+')] if '+' in tag_str else [tag_str]
                for part in parts:
                    key, value = parse_tag(part)
                    if not key:
                        continue
                    var_name = tag_keys[key]
                    if not value:
                        continue
                    if value == '*' or value.startswith('*'):
                        combo_conditions.append(f"!{var_name}_.empty()")
                    else:
                        value_clean = value.replace('*', '')
                        if value_clean:
                            combo_conditions.append(f'{var_name}_ == "{value_clean}"sv')
            if combo_conditions:
                if len(combo_conditions) == 1:
                    cond_expr = combo_conditions[0]
                else:
                    cond_expr = ' && '.join(combo_conditions)
                conditions.append(
                    f"if ({cond_expr}) return amenity_category::{entry['enum_name']};"
                )
        entry['conditions'] = conditions

    return template.render(
        entries=entries,
        sanitize_enum_name=sanitize_enum_name,
        tag_keys=tag_keys,
        member_vars=member_vars
    )

def main():
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python generate_osm_parser.py <html_file> [output_file]")
        print("  html_file: Path to OSM wiki HTML table file")
        print("  output_file: Output C++ header file (default: amenity_category.h)")
        sys.exit(1)
    
    html_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else "amenity_category.h"
    
    # Read HTML content
    print(f"Reading {html_file}...")
    with open(html_file, 'r', encoding='utf-8') as f:
        html_content = f.read()
    
    # Extract entries (one per SVG/icon row)
    print("Parsing HTML...")
    entries = extract_categories(html_content)

    print(f"\nFound {len(entries)} amenity entries (one per SVG/icon row)\n")

    # Generate C++ header
    print("Generating C++ code...")
    cpp_code = generate_cpp_header(entries)
    
    # Write to file
    with open(output_file, 'w') as f:
        f.write(cpp_code)
    
    print(f"✓ Generated {output_file}")
    print(f"\nUsage in C++:")
    print(f"  osr::amenity_tags tags(osm_object);")
    print(f"  auto category = tags.get_category();")
    print(f"  std::cout << osr::to_str(category) << '\\n';")

if __name__ == "__main__":
    main()
