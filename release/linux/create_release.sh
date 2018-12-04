#!/bin/bash

# Script to create a release of OpenHAM for Linux

# build solver

VERSION=0.3

cd ../../build/cmake && 
#rm -rf bb-gcc && 
#./build.sh 2 release &&
cd - &&
rm -rf OpenHam-$VERSION &&
mkdir OpenHam-$VERSION &&
cd OpenHam-$VERSION &&
cp ../../../bin/release/OpenHAMSolver . &&
mkdir examples &&
cp -r ../../../data examples/ &&
cd .. &&

echo '*** Created release for version '$VERSION' ***'







