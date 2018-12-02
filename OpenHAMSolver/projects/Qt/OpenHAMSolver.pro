# Project file for OpenHAMSolver
# remember to set DYLD_FALLBACK_LIBRARY_PATH on MacOSX
# set LD_LIBRARY_PATH on Linux

TARGET = OpenHAMSolver
TEMPLATE = app

# this pri must be sourced from all our applications
include( ../../../externals/IBK/projects/Qt/IBK.pri )

QT -= core gui
CONFIG += console
CONFIG -= app_bundle

LIBS += \
	-lDelphinLight \
	-lCCM \
	-lDataIO \
	-lIBK \
	-lTiCPP

INCLUDEPATH = \
	../../../externals/DataIO/src \
	../../../externals/IBK/src \
	../../../externals/DelphinLight/src

SOURCES += \
	../../src/main.cpp \
	../../src/Model.cpp \
	../../src/Integrator.cpp \
	../../src/Outputs.cpp \
	../../src/Material.cpp \
	../../src/BandMatrix.cpp \
	../../src/Directories.cpp \
	../../src/SolverFeedback.cpp \
	../../src/Mesh.cpp
HEADERS += \
	../../src/Model.h \
	../../src/Integrator.h \
	../../src/Outputs.h \
	../../src/Material.h \
	../../src/BandMatrix.h \
	../../src/Directories.h \
	../../src/SolverFeedback.h \
	../../src/Mesh.h

DISTFILES += \
	../../../data/tests/test01_steady_state/steady_state.d6p
