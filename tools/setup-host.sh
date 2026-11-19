#!/bin/bash
# Host setup for ReflexBoard (same tools as the course tutorial Tutorial-HPS-BuildSystem),
# for Ubuntu or WSL.
# Installs: build tools, gdb-multiarch, Linaro GCC 7.4.1, Intel SoCEDS 18.1,
# and adds the environment variables to ~/.bashrc.
#
# Run from inside Ubuntu:  bash tools/setup-host.sh
set -e

WORK=~/work
ALTERAPATH=~/intelFPGA/18.1
LINARO=gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf
# releases.linaro.org no longer serves the old toolchains; TechNexion mirrors the same file
LINARO_URL=https://download.technexion.com/development_resources/development_tools/gcc/$LINARO.tar.xz
SOCEDS=SoCEDSSetup-18.1.0.625-linux.run
SOCEDS_SHA1=b02e4e933dbfa32fa9e115b178449d210e4829a1
# Altera's download site needs a login, so the installer is downloaded with the browser
SOCEDS_PAGE=https://www.altera.com/downloads/embedded-tools/soc-fpga-embedded-development-suite-soc-eds-standard-edition-software-version-18-1-linux

echo "== apt packages"
sudo apt update
sudo apt install -y build-essential git wget xz-utils gdb-multiarch openssh-client python3

mkdir -p "$WORK"

echo "== Linaro GCC"
if [ ! -d "$WORK/$LINARO" ]; then
    # drop a broken/partial download (e.g. an HTML page saved by a redirect)
    xz -t "$WORK/$LINARO.tar.xz" 2>/dev/null || rm -f "$WORK/$LINARO.tar.xz"
    wget -c -P "$WORK" "$LINARO_URL"
    xz -t "$WORK/$LINARO.tar.xz"
    tar xf "$WORK/$LINARO.tar.xz" -C "$WORK"
fi

echo "== Intel SoCEDS 18.1 (2.6 GB installer)"
if [ ! -d "$ALTERAPATH/embedded" ]; then
    if [ ! -f "$WORK/$SOCEDS" ]; then
        # Linux Downloads folder, or (in WSL) any Windows user's Downloads folder
        FOUND=""
        for f in ~/Downloads/$SOCEDS /mnt/c/Users/*/Downloads/$SOCEDS; do
            [ -f "$f" ] && FOUND="$f" && break
        done
        if [ -z "$FOUND" ]; then
            echo "Download $SOCEDS with your browser from:"
            echo "  $SOCEDS_PAGE"
            echo "into your Downloads folder, then run this script again."
            exit 1
        fi
        cp "$FOUND" "$WORK/"
    fi
    echo "$SOCEDS_SHA1  $WORK/$SOCEDS" | sha1sum -c \
        || echo "WARNING: SHA1 differs from Intel's published value - incomplete download?"
    chmod +x "$WORK/$SOCEDS"
    "$WORK/$SOCEDS" --mode unattended --accept_eula 1 --installdir "$ALTERAPATH"
fi

echo "== ~/.bashrc"
if ! grep -q "INTEL FPGA QUARTUS 18.1" ~/.bashrc; then
    cat >> ~/.bashrc <<EOF

# INTEL FPGA QUARTUS 18.1
export ALTERAPATH=$ALTERAPATH
export PATH=\$PATH:\${ALTERAPATH}/embedded/
export PATH=\$PATH:\${ALTERAPATH}/embedded/host_tools/altera/preloadergen/
export SOCEDS_DEST_ROOT=\${ALTERAPATH}/embedded
export SOCEDS_HWLIB=\${ALTERAPATH}/embedded/ip/altera/hps/altera_hps/hwlib/

# GCC Linaro on path
export GCC_Linaro=$WORK/$LINARO/bin
export PATH=\$PATH:\${GCC_Linaro}
EOF
fi

echo "== check"
"$WORK/$LINARO/bin/arm-linux-gnueabihf-gcc" --version | head -1
ls "$ALTERAPATH/embedded/ip/altera/hps/altera_hps/hwlib/include/hwlib.h"
echo "Done. Open a new terminal (or run: source ~/.bashrc)."
