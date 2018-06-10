#/bin/sh

INCLUDE_TARGET=../../include
LIB_TARGET=../../lib

mkdir -p $INCLUDE_TARGET
cp -f -R fly $INCLUDE_TARGET

mkdir -p $LIB_TARGET
cp libfly_os2pm.a $LIB_TARGET/fly_os2pm.a
cp libfly_os2vio.a $LIB_TARGET/fly_os2vio.a
cp libfly_os2x.a $LIB_TARGET/fly_os2x.a
cp libfly_os2x11.a $LIB_TARGET/fly_os2x11.a

