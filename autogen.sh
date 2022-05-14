#!/bin/sh

set -e

srcdir="$(dirname "$0")"

# "$srcdir"/bootstrap.sh
# if [ -z "$NOCONFIGURE" ]; then
#     exec "$srcdir"/configure --enable-examples-build --enable-tests-build "$@"
# fi

if [ ! -d m4 ]; then
    mkdir m4
fi
exec autoreconf -ivf   
