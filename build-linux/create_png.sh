#!/bin/sh

gprof2dot --format=callgrind --output=out.dot $1 &&
dot -Tpng out.dot -o graph.png
