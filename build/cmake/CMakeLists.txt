# CMakeLists.txt file for OpenHAMSolverBundle library and applications
# Author: Andreas Nicolai (andreas.nicolai -at- tu-dresden.de)

# The project name
project( OpenHAMSolverBundle )

# Require a fairly recent cmake version
cmake_minimum_required( VERSION 2.8.12 )

# -------------------------------------------------------------
# Initial commands
# -------------------------------------------------------------

# Uncomment this to enable detailed make output
#set( CMAKE_VERBOSE_MAKEFILE ON )

# Hide some more cache variables to keep things tidy
MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)
MARK_AS_ADVANCED(EXECUTABLE_OUTPUT_PATH LIBRARY_OUTPUT_PATH)

# Set default build type
if (NOT CMAKE_BUILD_TYPE)
	set( CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# on Unix we want really detailed warnings
if (UNIX)
	ADD_DEFINITIONS( -Wall -fPIC)
endif (UNIX)

# add VC compiler-specific flags (NONMINMAX define and disable "unsafe" warnings)
if (MSVC)
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D\"NOMINMAX\" /wd4996 /GS-" )
endif (MSVC)


# -------------------------------------------------------------
# Subdirectories and Subprojects
# -------------------------------------------------------------

add_subdirectory( ../../externals/CCM/projects/cmake_local CCM )
add_subdirectory( ../../externals/IBK/projects/cmake_local IBK )
add_subdirectory( ../../externals/DataIO/projects/cmake_local DataIO)
add_subdirectory( ../../externals/TiCPP/projects/cmake_local TiCPP)
add_subdirectory( ../../externals/DelphinLight/projects/cmake_local DelphinLight)

# add subdirectories for all builds
add_subdirectory( ../../OpenHAMSolver/projects/cmake OpenHAMSolver )


