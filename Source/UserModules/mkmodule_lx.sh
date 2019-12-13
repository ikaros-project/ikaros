#!/bin/bash

# usage: sh mkmodule.sh modulename

MODULENAME="$1"
ORGNAME="TrondTemplate"
## make new dir with MODULENAME
#mkdir $MODULENAME
## copy everything from MyModule
cp -R $ORGNAME $MODULENAME
cd $MODULENAME
ls
## rename all files
for file in $(find . -type f -maxdepth 2 -name "*$ORGNAME*")
    do
        echo '    change' "$file" 'to' `echo "$file" | sed s/$ORGNAME/$MODULENAME/g`
        #mv "$file" `echo "$file" | sed s/$ORGNAME/$MODULENAME/g`
        #mv "$file" "$(echo '$file'| sed -e '/$ORGNAME/$MODULENAME/g')"
        mv "$file" "`echo $file | sed "s/$ORGNAME/$MODULENAME/g"`";
    done
ls
# search and replace in files
sed -e "s/$ORGNAME/$MODULENAME/; s/~$ORGNAME/~$MODULENAME/; s/\&$ORGNAME/\&$MODULENAME/; s:/$ORGNAME:/$MODULENAME:g;" -i  *.*
cd Examples/
ls
sed -e "s/$ORGNAME/$MODULENAME/; s/~$ORGNAME/~$MODULENAME/; s/\&$ORGNAME/\&$MODULENAME/; s:/$ORGNAME:/$MODULENAME:g;" -i  *.*
cd ../..
sed -i  -e '3i\add_subdirectory('"$MODULENAME"')\n' CMakeLists.txt
echo "Done"