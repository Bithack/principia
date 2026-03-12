#!/bin/bash

# This script is used to update the sandbox menu textures. It is automatically run by the game after all the objects
# have been rendered. This script will then process them into the final textures used by the game.

# To begin the process, press S in the main menu on a debug Linux build of the game. You should have gamma correction
# disabled to generate correct colours.

NUM_TEXTURES=10

for ((i = 0; i < $NUM_TEXTURES; i ++)); do
    cp sandbox-menu-$i.png tmp.png
    cp tmp.png mask.png
    magick tmp.png -alpha Off mask.png -compose CopyOpacity -composite PNG32:tmp_alpha.png
    magick tmp_alpha.png -background black -shadow 80x3+0+0 tmp_shadow.png

    magick tmp_alpha.png tmp_shadow.png -compose DstOver +repage -gravity center +repage -composite tmp_composite.png
    magick tmp_composite.png -background 'rgb(54,54,54)' -flatten -alpha Off ./data/textures/sandbox-menu-$i.png

    rm tmp.png mask.png tmp_alpha.png tmp_shadow.png
    rm tmp_composite.png
    rm sandbox-menu-$i.png
done

magick items.png +clone -background black -shadow 100x3+0+0 -composite items_shadow.png
magick items.png items_shadow.png -compose DstOver +repage -gravity center +repage -composite ./data/textures/menu_items.png
rm items.png items_shadow.png
