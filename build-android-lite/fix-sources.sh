while true; do
    read -p "Warning: This will remove any changes you have made to the lite sourcefiles. Are you sure you want to continue? [y/n] " yn
    case $yn in
        [Yy]* ) break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done

rm -rfv src/
cp -rv ../build-android/src .

find . -type f -iname "*.java" -exec sed -i 's/com\.bithack\.principia\.R/com.bithack.principialite.R/g' {} \;
find . -type f -iname "*.java" -exec sed -i 's/com\.bithack\.principia\.PrincipiaActivity/com.bithack.principialite.PrincipiaActivity/g' {} \;
find . -type f -iname "*.java" -exec sed -i 's/package com\.bithack\.principia\;/package com.bithack.principialite;/g' {} \;
find . -type f -iname "*.java" -exec sed -i 's/FULL = true/FULL = false/g' {} \;

mkdir -pv src/com/bithack/principialite
mv src/com/bithack/principia/*.java src/com/bithack/principialite/

echo "done"

#bottom two might be necessary if the third one does not work.
#find . -type f -iname "*.java" -exec sed -i 's/package com\.bithack\.principia/package com.bithack.principialite/g' {} \;
#find . -type f -iname "*.java" -exec sed -i 's/package com\.bithack\.principialite\.shared/package com.bithack.principia.shared/g' {} \;
