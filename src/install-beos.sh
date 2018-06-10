#/bin/sh

INCLUDE_TARGET=../../include
LIB_TARGET=../../lib

mkdir -p $INCLUDE_TARGET
cp -f -R fly $INCLUDE_TARGET

mkdir -p $LIB_TARGET
for a in $1 $2 $3 $4 $5;
do
    cp -f $a $LIB_TARGET
done
