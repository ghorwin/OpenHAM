#!/bin/bash

# Script to create a release of OpenHAM for Linux

VERSION=0.3

cd ../../build/cmake && 
rm -rf bb-gcc && 
./build.sh 2 release skip-test &&
cd - &&
rm -rf OpenHam-$VERSION &&
mkdir OpenHam-$VERSION &&
cd OpenHam-$VERSION &&
cp ../../../bin/release/OpenHAMSolver . &&
cp ../../../LICENSE . &&
cp ../../../AUTHORS . &&
cp -r ../../../data/examples . &&
cd .. && 
7za a -t7z OpenHam-$VERSION.7z OpenHam-$VERSION/* &&

echo '*** Created release for version '$VERSION' ***'


