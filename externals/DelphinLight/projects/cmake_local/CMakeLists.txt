# CMakeLists.txt file

project( DelphinLight )

# collect a list of all source files of the library
file( GLOB DelphinLight_LIB_SRCS ${PROJECT_SOURCE_DIR}/../../src/*.cpp )

# add include directories
include_directories(
	${PROJECT_SOURCE_DIR}/../../../IBK/src 
	${PROJECT_SOURCE_DIR}/../../../TiCPP/src 
)

# build the library
add_library( DelphinLight STATIC
	${DelphinLight_LIB_SRCS}
)

