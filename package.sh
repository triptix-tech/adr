#!/bin/bash

APP=adr
GIT_SHA=`git --no-pager log -1 --format=%h`
VERSION=`git describe --always --tags --dirty=-${GIT_SHA}`
ARCH=`uname -m`

if [[ $ARCH == aarch64 ]]
then
  #echo -- rewrite arch from ${ARCH} to arm64
  ARCH=arm64
fi

if [[ $ARCH == x86_64 ]]
then
  #echo -- rewrite arch from ${ARCH} to amd64
  ARCH=amd64
fi

if [[ $VERSION == v* ]]
then
  #echo -- strip version from ${VERSION} to ${VERSION:1}
  VERSION=${VERSION:1}
fi

DIR=${APP}_${VERSION}_${ARCH}

if [[ $1 == name ]]
then
  echo ${DIR}.deb
  exit 0
fi

echo -- app=${APP}, version=${VERSION}, arch=${ARCH}, DIR=${DIR}

mkdir -p ${DIR}/usr/local/bin
cp build/adr-extract ${DIR}/usr/local/bin
cp build/adr-server ${DIR}/usr/local/bin

mkdir -p ${DIR}/etc/systemd/system
cat <<EOT > ${DIR}/etc/systemd/system/adr.service
[Unit]
Description=ADR
After=network.target

[Service]
Restart=always
RestartSec=15
TimeoutStartSec=300
User=adr
Group=adr
WorkingDirectory=/opt/adr
ExecStart=/opt/adr/adr-server /opt/typeahead.bin

[Install]
WantedBy=multi-user.target
EOT

mkdir -p ${DIR}/DEBIAN
cat <<EOT > ${DIR}/DEBIAN/control
Package: ${APP}
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: triptix GmbH <info@triptix.tech>
Depends: adduser, libc6 (>= 2.34), libstdc++6 (>= 12.0.0)
Description: ${APP}
EOT

cat <<EOT > ${DIR}/DEBIAN/postinst
#!/bin/sh -e

if ! getent group adr >/dev/null 2>&1; then
    addgroup --system --quiet adr
fi
if ! getent passwd adr >/dev/null 2>&1; then
    adduser --system --quiet --ingroup adr \\
        --no-create-home --home /nonexistent \\
        adr
fi
EOT

chmod +x ${DIR}/DEBIAN/postinst

dpkg-deb --build --root-owner-group ${DIR}