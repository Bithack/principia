#!/bin/bash

# This script relies on windows_release.sh being run to prepare the release/ directory

touch release/portable.txt

cp ../packaging/play_community_level.bat release/

cp -r ../data-{pc,shared}/ release/

# juggling time
mv principia.exe principia_exe.exe
mv release/ Principia/

7z a principia-portable.7z Principia/

# switch it back
mv Principia/ release/
mv principia_exe.exe principia.exe
