# CMakeLists.txt file for OpenHAMSolver
# Does not work as stand-alone CMakeLists.txt file, must be included in CMakeLists.txt master file.

project( OpenHAMSolver )

# add include directories
include_directories(
	${PROJECT_SOURCE_DIR}/../../src
)

# set variable for dependent libraries
set( LINK_LIBS
	${APPLE_FRAMEWORKS}
)

# add solver executable
add_executable( ${PROJECT_NAME}
	${PROJECT_SOURCE_DIR}/../../src/main.cpp
)

# link against the dependent libraries
target_link_libraries( ${PROJECT_NAME} 
	${LINK_LIBS}
)


