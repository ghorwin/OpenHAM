# -------------------------------------------------
# Project for DelphinLight library
# -------------------------------------------------
TARGET = DelphinLight
TEMPLATE = lib

# this pri must be sourced from all our libraries,
# it contains all functions defined for casual libraries
include( ../../../IBK/projects/Qt/IBK.pri )

unix|mac {
	VER_MAJ = 1
	VER_MIN = 0
	VER_PAT = 0
	VERSION = $${VER_MAJ}.$${VER_MIN}.$${VER_PAT}
}

LIBS += -lIBK -lTiCPP

INCLUDEPATH = ../../../IBK/src \
	../../../TiCPP/src

SOURCES += \
        ../../src/DELPHIN_Project.cpp \
	../../src/DELPHIN_MaterialReference.cpp

HEADERS += \
        ../../src/DELPHIN_Project.h \
	../../src/DELPHIN_MaterialReference.h
