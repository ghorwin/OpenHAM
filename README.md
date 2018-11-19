# OpenHAM Wiki

## Introduction ##
OpenHAM is an open-source Heat, Air and Moisture transport model, initially written by *Andreas Nicolai (andreas.nicolai -at- tu-dresden.de)*. It is meant as an **open source reference implementation** of the governing equations that are standardized, for example, in the HAMSTAD modeling document and the EN 15026 standard. It is specialized for 1D geometries and includes an automated grid generation algorithm.

Also, it is a testing development to try out and demonstrate various solution methods for solving the coupled balance equations. 

OpenHAM is distributed under an open source license, so that the engine can be freely used and HAM solvers do not need to be black-boxes anylonger. Also, future master/PhD students won't have to spent too much time by redoing the same mistakes as many others before and wasting precious research time by writing and debugging basic solver code.

The code base is based on the PhD thesis of Andreas Nicolai and uses data and output file formats of the commercial [[DELPHIN simulation software](http://bauklimatik-dresden.de)].

## OpenHAM Releases ##

OpenHAM is a console application and does not require an installer. Simply extract the archives
into some subdirectory and run the solver.

You may want to use the `--help` command line argument to see available options:


`OpenHAMSolver --help`


Please post bugs/feature requests via the ticket system.

### Linux Release Notes ###

The Linux binary was compiled on Ubuntu 16.04.3 LTS and requires the usual C/C++/GCC libraries.
On different Linux distros, simply recompile the solver - it does not have any dependencies
beside the usual *built-essentials* package.



## Modeling and Data Structures ##
* physical model conforms to that of DELPHIN, or HAMSTAD modeling document, or EN 15026 (2017) (for details, see publication)
    + [Material model](Material model) and supported functions/built-in functions
* input data files follow format conventions of the DELPHIN software: d6p-files for projects/constructions, m6-files for materials, This allows exchange of data files between OpenHAM and DELPHIN and allows reusing some implementation functionality. Details for input file formats are listed in:
    + [Material data file format](Material data files)
    + [OpenHAM project file format](OpenHAM project files) (actually a subset of DELPHIN 6 input files)
* output files are written in DataIO standard, which allows use of the free PostProcessing software from IBK (see [[Bauklimatik Dresden](http://bauklimatik-dresden.de)]. A converter utility to extract data from DataIO containers and/or convert to other formats (CSV, TecPlot) is available.
    + see [Output handling](Outputs)

### Material Functions ###
* either use functions from m6 material file (only a subset of all options is supported)
* use built-in functions from Material class, in this case the material reference in the project file must have the name: `built-in:<nr>`

Example:

    :::xml
	<!--List of all materials used in this construction-->
	<Materials>
		<MaterialReference name="My material">built-in:2</MaterialReference>
	</Materials>

This will make the solver use the built-in material data set with index 2 (see Material class).

*OpenHAM* supports a subset of the material functions used in DELPHIN when providing material data files. Each file must include the following functions:

* moisture retention function thetal(pc) as spline
* liquid conductivity as spline
* vapour permeability as spline

Moisture-tight materials have to be flagged as such by VAPOR_TIGHT or WATER_TIGHT flags (see [Material data file format](Material data files)).

## Numerical Algorithm ##

* Automatic grid generation (equidistant or variable)
* Implicit Euler Time Integration
* Newton-Rapshon solution for non-linear equation systems
    + different options for modified Newton-Raphson: strict, once-per-step, threshhold-based
* Banded matrix for solving linear equation systems
* Different options for selecting the solution variable
* Time step adaptation based on convergence properties and local error control

Details see publication (once done).

## Developing ##
Suggested is the use of Qt Creator as development environment, with gcc compiler on Linux/Mac and gcc/VC on Windows.

* [Qt Creator Editor Settings](Qt Creator Editor Settings)
* [Coding-Guidelines](Coding-Guidelines)

### Building via command line ###

Building the solver is automated via command line scripts:

On Linux/Mac:

    :::bash
    cd build/cmake
    # build with 4 CPUs in release mode
    ./build.sh 4 release


On Windows:

    :::batch
    cd build/cmake
    # build with 4 CPUs in release mode
    build.bat 4 release

### Deployment ###

OpenHAM is just a single command line executable, statically built against dependend libraries. All you need to do is copy the executable.

On Windows you may need to copy the following DLLs alongside the executable, unless already installed on the target machine (via VC redistributable or similar).

**When building with VC 2015**:

* `msvcp140.dll`
* `vcruntime140.dll`

**When building with GCC**:

* `libgcc_s_dw2-1.dll`
* `libstdc++-6.dll`
* `libwinpthread-1.dll`

## Publications/References ##

Vogelsang S. and Nicolai A.; *Delphin 6 Output File Specification*; http://nbn-resolving.de/urn:nbn:de:bsz:14-qucosa-201373; 2016

Vogelsang S., Fechner H. and Nicolai A.; *Delphin 6 Material File Specification*; http://nbn-resolving.de/urn:nbn:de:bsz:14-qucosa-126274; 2013

(in preparation)
Nicolai A.; *An open-source hygrothermal transport solver reference implementation according to EN 15026*


-----
Editing: The wiki uses [Markdown](/p/openham/wiki/markdown_syntax/) syntax.

[[members limit=20]]
[[download_button]]


