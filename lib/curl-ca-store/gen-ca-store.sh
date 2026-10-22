#!/bin/bash
set -e

VER="2026-10-20"
HASH="796f1389bfea13c0ed78f663b765baaff54a508e5483786bd6ece73c5e3e3cbe"

URL="https://curl.se/ca/cacert-${VER}.pem"
OUTPUT="cacert.h"
TMP="$(mktemp)"

trap 'rm -f "$TMP"' EXIT

curl --fail --location --silent --show-error "$URL" -o "$TMP"

sha256sum -c <(echo "$HASH  $TMP") || { echo "Hash mismatch for CA store"; exit 1; }

{
    echo '#pragma once'
    echo
    echo 'static const char cacert_pem[] = {'
    sed 's/\\/\\\\/g; s/"/\\"/g; s/$/\\n"/; s/^/"/' "$TMP"
    echo '};'
} > "$OUTPUT"

echo "Generated $OUTPUT"
