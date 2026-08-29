#!/bin/sh
# Build helper to create a static temper_query binary in an ephemeral Alpine
# build container.

set -euo pipefail

platform="linux/x86-64"
#platform="linux/arm64/v8"

# This 
build() {
    local host_user_group="$1"
    shift

    apk update
    apk add --no-cache gcc musl-dev libudev-zero-dev hidapi-dev
    cd /host
    gcc -static -Wl,-static \
        -o temper_query src/temper_query.c \
        /usr/lib/libhidapi-hidraw.a  \
        /usr/lib/libhidapi-libusb.a \
        /usr/lib/libudev.a
    strip temper_query
    chown -R "$host_user_group" temper_query
    echo
    echo "Build success: temper_query"
    echo
}
# --

if [ "${1:-}" = "build" ] ; then
    shift
    build ${@}
    exit
fi

platform="${1:-$platform}" 
uid="$(id -u)"
gid="$(id -g)"

exec docker run -i --rm \
         -v "$(pwd):/host" \
         --platform "${platform}" \
         alpine \
         /host/container-build.sh "build" "${uid}:${gid}"
