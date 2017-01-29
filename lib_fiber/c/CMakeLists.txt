cmake_minimum_required(VERSION 2.8)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

message(${PROJECT_SOURCE_DIR})

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

set(rpath lib_fiber/c)
set(src ${PROJECT_SOURCE_DIR}/${rpath}/src)
set(acl_path ${PROJECT_SOURCE_DIR}/lib_acl)
set(acl_include ${acl_path}/include)
message(${acl_include})

include_directories(
	${PROJECT_SOURCE_DIR}/${rpath}/src
	${PROJECT_SOURCE_DIR}/${rpath}/include
	${acl_include}
	${acl_path}/src/master
)

set(sources
	${src}
	${src}/master
)

foreach(iter ${sources})
	aux_source_directory(${iter} lib_src)
endforeach()

set(lib_output_path ${PROJECT_BINARY_DIR}/lib)

set(LIBRARY_OUTPUT_PATH ${lib_output_path})
add_library(_fiber_static STATIC ${lib_src})
SET_TARGET_PROPERTIES(_fiber_static PROPERTIES OUTPUT_NAME "_fiber")

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-rpath,. -L${lib_output_path} -l_acl")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})
add_library(_fiber SHARED ${lib_src})
link_directories(${lib_output_path})
target_link_libraries(_fiber _acl)

##############################################################################
