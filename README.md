# OpenHAM

OpenHAM is an open-source Heat, Air and Moisture transport model, initially written by *Andreas Nicolai (andreas.nicolai -at- gmx.net)* (formerly at Institut für Bauklimatik, TU Dresden, Germany). 
It is meant as an **open source reference implementation** of the governing equations that are standardized, for example, in the HAMSTAD modeling document and the EN 15026 standard. It is specialized for 1D geometries and includes an automated grid generation algorithm.

See [OpenHAM-wiki](../../wiki) for details.


## Building

Clone this repository and run one of the build scripts in `build/cmake`.

### Linux

```bash
cd build/cmake
# build using 4 CPUs in release mode
./build.sh 4 release
```

### Windows

```batch
cd build\cmake
:: build with VC 2019 x64
build_VC_x64.bat
```

For other VC versions, just copy the batch file and adjust paths.


## Development

Open `OpenHAMSession.pro` in Qt Creator and you're ready to go.

----
[![Github All Releases](https://img.shields.io/github/downloads/ghorwin/OpenHAM/total.svg)](https://github.com/ghorwin/OpenHAM/releases)

