#!/bin/bash
set -e

VER="2026-08-13"
HASH="f66dff1bdf8f96060b8177976f8b7d9254bc89bc4db933d769f7384d28480bc9"

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
