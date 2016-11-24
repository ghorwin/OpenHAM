# Project file for OpenHAMSolver
# remember to set DYLD_FALLBACK_LIBRARY_PATH on MacOSX
# set LD_LIBRARY_PATH on Linux

TARGET = OpenHAMSolver

TEMPLATE = app
QT -= core gui
CONFIG += console
CONFIG -= app_bundle

CONFIG(debug, debug|release) {
	OBJECTS_DIR = debug
	DESTDIR = ../../../bin/debug
}
else {
	OBJECTS_DIR = release
	DESTDIR = ../../../bin/release
}

INCLUDEPATH = \
	../../../externals/DataIO/src \
	../../../externals/IBK/src

LIBS += \
	-L../../../externals/lib \
	-lDataIO \
	-lIBK

SOURCES += \
	../../src/main.cpp \
	../../src/Model.cpp \
	../../src/Integrator.cpp \
	../../src/Outputs.cpp \
    ../../src/Material.cpp \
    ../../src/BandMatrix.cpp
HEADERS += \
	../../src/Model.h \
	../../src/Integrator.h \
	../../src/Outputs.h \
    ../../src/Material.h \
    ../../src/BandMatrix.h
