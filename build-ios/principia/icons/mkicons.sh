#!/bin/bash

SIZES=(29x29 58x58 80x80 40x40 57x57 114x114 120x120 50x50 100x100 72x72 144x144 76x76 152x152)

for t in "${SIZES[@]}"
do
    convert icon-new-512.png -resize ${t} icon-${t}.png
done
