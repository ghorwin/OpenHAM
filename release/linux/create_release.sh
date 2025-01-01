#!/bin/bash

# Script to create a release of OpenHAM for Linux

VERSION=0.3.1
DISTRO=Ubuntu-24.04

cd ../../build/cmake && 
rm -rf bb-gcc && 
./build.sh 2 release skip-test &&
cd - &&
rm -rf OpenHAM-$VERSION-$DISTRO &&
mkdir OpenHAM-$VERSION-$DISTRO &&
cd OpenHAM-$VERSION-$DISTRO &&
cp ../../../bin/release/OpenHAMSolver . &&
cp ../../../LICENSE . &&
cp ../../../AUTHORS . &&
cp -r ../../../data/examples . &&
cd .. && 
7za a -t7z OpenHAM-$VERSION-$DISTRO.7z OpenHAM-$VERSION-$DISTRO/* &&

echo '*** Created release for version '$VERSION' ***'


