#!/bin/bash

DIR=${1}
if [ -z $DIR ]; then
   echo "usage: html_page.sh <dir>"
   exit 1
fi

echo "<html><head><title>Benchmarks</title></head><body>" > $DIR/index.html
for x in ${DIR}/*; do
   base=$(basename $x)
   if [ ! -d $x ]; then
      continue
   fi
   echo "<h1>$base</h1>" >> $DIR/index.html
   for img in `ls -v $x/*.png`; do
      name=$(basename $img .png)
      echo "<img src=\"$img\" weight=\"200\" height=\"200\" />" >> $DIR/index.html
   done
done
echo "</body></html>" >> $DIR/index.html
