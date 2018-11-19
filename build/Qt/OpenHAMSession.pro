# Project for OpenHAMSolver session and all IBK libraries

TEMPLATE=subdirs

# SUBDIRS lists all subprojects
SUBDIRS += OpenHAMSolver \
	CCM \
	DelphinLight \
	DataIO \
	IBK \
	TiCPP

# where to find the sub projects
OpenHAMSolver.file = ../../OpenHAMSolver/projects/Qt/OpenHAMSolver.pro

CCM.file = ../../externals/CCM/projects/Qt/CCM.pro
DelphinLight.file = ../../externals/DelphinLight/projects/Qt/DelphinLight.pro
DataIO.file = ../../externals/DataIO/projects/Qt/DataIO.pro
IBK.file = ../../externals/IBK/projects/Qt/IBK.pro
TiCPP.file = ../../externals/TiCPP/projects/Qt/TiCPP.pro

# dependencies
OpenHAMSolver.depends = DelphinLight CCM DataIO IBK TiCPP

CCM.depends = IBK TiCPP
DataIO.depends = IBK
DelphinLight.depends = IBK TiCPP
TiCPP.depends = IBK
