#!/bin/bash

NUM_TEXTURES=10

for ((i = 0; i < $NUM_TEXTURES; i ++)); do
  convert sandbox-menu-$i.png tmp.png
#convert tmp.png -colorspace HSB -separate mask.png
  convert tmp.png mask.png
  convert tmp.png -alpha Off mask.png -compose CopyOpacity -composite PNG32:tmp_alpha.png
  convert tmp_alpha.png -background black -shadow 80x3+0+0 tmp_shadow.png

  convert tmp_alpha.png tmp_shadow.png -compose DstOver +repage -gravity center +repage -composite tmp_composite.png
  convert tmp_composite.png -background 'rgb(54,54,54)' -flatten -alpha Off ../data/textures/sandbox-menu-$i.png
#gimp ../data/textures/sandbox-menu-$i.jpg

  rm tmp.png mask.png tmp_alpha.png tmp_shadow.png
  rm tmp_composite.png
  rm sandbox-menu-$i.png
done

#convert items.bmp tmp.png
#convert tmp.png -colorspace HSB -separate mask.png
#convert tmp.png mask.png

#convert tmp.png -alpha Off mask.png -compose CopyOpacity -composite PNG32:tmp_alpha.png
#convert tmp_alpha.png -background black -shadow 80x3+0+0 tmp_shadow.png

#convert tmp_alpha.png tmp_shadow.png -compose DstOver +repage -gravity center +repage -composite tmp_composite.png
#convert tmp_composite.png -background 'rgb(54,54,54)' -flatten -alpha Off ../data/textures/items.jpg
#cp tmp_composite.png ../data/textures/items.png

convert items.png +clone -background black -shadow 100x3+0+0 -composite test.png
convert items.png test.png -compose DstOver +repage -gravity center +repage -composite ../data/textures/menu_items.png

#convert items.png \( +clone -background black -shadow 100x3+0+0 \)\
#            -compose DstOver +swap -background none -layers merge -gravity center +repage ../data/textures/items.png

#rm tmp.png mask.png tmp_alpha.png tmp_shadow.png
#rm tmp_composite.png
#rm items.bmp
