 cat Makefile | grep -e ".lo:" | awk '{print "../src/SDL/" $2}' | sed 's/\/\.\//\//g'
