#!/bin/sh

make distclean
qmake -project
mv OpenOCD-QtGUI.pro OpenOCD-QtGUI.pro.tmp
cat OpenOCD-QtGUI.pro.tmp | sed 's/\#\ Input/QT\ +=\ network/g' > OpenOCD-QtGUI.pro
qmake
make 
