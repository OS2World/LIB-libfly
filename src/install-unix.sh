#/bin/sh

# root?

IS_ROOT=`id | grep "uid=0(root)" | wc -l`

if [ $IS_ROOT -eq 1 ]
then
  echo "libfly system-wide installation"
  INCLUDE_TARGET=/usr/include
  LIB_TARGET=/usr/lib
else
  echo "libfly private installation"
  INCLUDE_TARGET=../../include
  LIB_TARGET=../../lib
fi

mkdir -p $INCLUDE_TARGET/fly
cp -f -R fly/* $INCLUDE_TARGET/fly

mkdir -p $LIB_TARGET
for a in $1 $2 $3 $4 $5;
do
    echo $LIB_TARGET/$a
    cp -f $a $LIB_TARGET
done
