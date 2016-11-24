# CMakeLists.txt file for application and dependent libraries

cmake_minimum_required( VERSION 2.6 )

project( OpenHAM )

# -------------------------------------------------------------
# Initial commands
# -------------------------------------------------------------

# Change to ON to enable detailed make output
set( CMAKE_VERBOSE_MAKEFILE OFF )

# Set release mode as default, except for the Borland Compiler
if (NOT CMAKE_BUILD_TYPE)
	set( CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# on Unix we want really detailed warnings
if (UNIX)
	ADD_DEFINITIONS( -Wall -fPIC)
endif (UNIX)

# for VC compiler, add the flag to disable wchar_t as native type
if (MSVC)
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:wchar_t-" )
endif (MSVC)

# -------------------------------------------------------------
# Packages
# -------------------------------------------------------------

# We need Qt for building
#find_package( Qt4  REQUIRED )

#set( QT_USE_QTXML TRUE )
#set( QT_USE_QTSVG TRUE )
#include( ${QT_USE_FILE} )


# -------------------------------------------------------------
# Subdirectories and Subprojects
# -------------------------------------------------------------

# add subdirectories for all builds
add_subdirectory( ../../OpenHAMSolver/projects/cmake OpenHAMSolver )


