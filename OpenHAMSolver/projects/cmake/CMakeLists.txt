# CMakeLists.txt file for OpenHAMSolver
# Does not work as stand-alone CMakeLists.txt file, must be included in CMakeLists.txt master file.

project( OpenHAMSolver )

# add include directories
include_directories(
	${PROJECT_SOURCE_DIR}/../../src
	${PROJECT_SOURCE_DIR}/../../../externals/CCM/src 
	${PROJECT_SOURCE_DIR}/../../../externals/IBK/src 
	${PROJECT_SOURCE_DIR}/../../../externals/DataIO/src 
	${PROJECT_SOURCE_DIR}/../../../externals/TiCPP/src 
	${PROJECT_SOURCE_DIR}/../../../externals/DelphinLight/src 
)

# set variable for dependent libraries
set( LINK_LIBS
	DelphinLight
	CCM
	DataIO
	TiCPP
	IBK
	${APPLE_FRAMEWORKS}
)

# collect a list of all source files
file( GLOB SRCS ${PROJECT_SOURCE_DIR}/../../src/*.cpp )

# add solver executable
add_executable( ${PROJECT_NAME} 
	${SRCS} 
)

# link against the dependent libraries
target_link_libraries( ${PROJECT_NAME} 
	${LINK_LIBS}
)


