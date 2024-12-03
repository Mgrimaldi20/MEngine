#!/bin/bash

usage()
{
	echo "Usage: $0 [debug|release]"
	exit 1
}

if [ "$#" -ne 1 ]; then
	usage
fi

case "$1" in
	debug)
		PRESET="linux-x64-debug"
		;;
	release)
		PRESET="linux-x64-release"
		;;
	*)
		usage
		;;
esac

cd "$(dirname "$0")"

cmake --preset $PRESET

make -C out/build/$PRESET

