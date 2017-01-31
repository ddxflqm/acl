cmake_minimum_required(VERSION 2.8)
#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

add_definitions(
	"-W"
	"-Wall"
	"-Werror"
	"-Wshadow"
	"-Wpointer-arith"
	"-Waggregate-return"
	"-Wmissing-prototypes"
	"-D_REENTRANT"
	"-D_USE_FAST_MACRO"
	"-DACL_WRITEABLE_CHECK"
	"-Wno-long-long"
	"-Wuninitialized"
	"-D_POSIX_PTHREAD_SEMANTICS"
	"-DACL_PREPARE_COMPILE"
	"-Wno-invalid-source-encoding"
	"-fPIC"
	"-Wstrict-prototypes"
)

if (CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "DEBUG")
else()
	string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
	add_definitions("-O3")
else()
	add_definitions("-g")
endif()

string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
if (CMAKE_SYSTEM_NAME MATCHES "LINUX")
	add_definitions("-DLINUX2")
elseif(CMAKE_SYSTEM_NAME MATCHES "ANDROID")
	add_definitions("-DACL_ARM_LINUX")
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

##############################################################################

set(rpath lib_protocol)
set(acl_path ${PROJECT_SOURCE_DIR}/lib_acl)
set(acl_include ${acl_path}/include)

include_directories(
	${acl_include}
	${PROJECT_SOURCE_DIR}/${rpath}/src
	${PROJECT_SOURCE_DIR}/${rpath}/include
)

set(src ${PROJECT_SOURCE_DIR}/${rpath}/src)
set(sources
	${src}
	${src}/http
	${src}/smtp
	${src}/icmp
)

foreach(iter ${sources})
	aux_source_directory(${iter} lib_src)
endforeach()

set(lib_output_path ${PROJECT_BINARY_DIR}/lib)

set(LIBRARY_OUTPUT_PATH ${lib_output_path})
add_library(_protocol_static STATIC ${lib_src})
SET_TARGET_PROPERTIES(_protocol_static PROPERTIES OUTPUT_NAME "_protocol")

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-rpath,. -L${lib_output_path} -l_acl")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})
add_library(_protocol SHARED ${lib_src})
link_directories(${lib_output_path})
target_link_libraries(_protocol _acl)

##############################################################################
