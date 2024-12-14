#!/bin/bash

usage()
{
	echo "Usage: $0 [linux|macos] [debug|release]"
	exit 1
}

if [ "$#" -ne 2 ]; then
	usage
fi

case "$2" in
	debug)
		case "$1" in
			linux)
				PRESET="linux-debug"
				;;
			macos)
				PRESET="macos-debug"
				;;
			*)
				usage
				;;
		esac
		;;
	release)
		case "$1" in
			linux)
				PRESET="linux-release"
				;;
			macos)
				PRESET="macos-release"
				;;
			*)
				usage
				;;
		esac
		;;
	*)
		usage
		;;
esac

cd "$(dirname "$0")"

cmake --preset $PRESET

cmake --build out/build/$PRESET

