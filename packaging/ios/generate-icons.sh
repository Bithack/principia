#!/usr/bin/env bash
set -eu

SRC="${1:-}"
OUT="${2:-icons}"

make_icon() {
	local size="$1"
	local filename="$2"

	echo "  ${size}x${size}  ${filename}"

	magick \
		"$SRC" \
		-background white \
		-alpha remove \
		-alpha off \
		-resize "${size}x${size}" \
		-strip \
		"$OUT/$filename"
}

echo "Generating iOS app icons..."

# iPhone
make_icon 40  "AppIcon-20@2x.png"
make_icon 60  "AppIcon-20@3x.png"
make_icon 58  "AppIcon-29@2x.png"
make_icon 87  "AppIcon-29@3x.png"
make_icon 80  "AppIcon-40@2x.png"
make_icon 120 "AppIcon-40@3x.png"
make_icon 120 "AppIcon-60@2x.png"
make_icon 180 "AppIcon-60@3x.png"

# iPad
make_icon 20  "AppIcon-20.png"
make_icon 40  "AppIcon-20@2x-ipad.png"
make_icon 29  "AppIcon-29.png"
make_icon 58  "AppIcon-29@2x-ipad.png"
make_icon 40  "AppIcon-40.png"
make_icon 80  "AppIcon-40@2x-ipad.png"
make_icon 76  "AppIcon-76.png"
make_icon 152 "AppIcon-76@2x.png"
make_icon 167 "AppIcon-83.5@2x.png"
