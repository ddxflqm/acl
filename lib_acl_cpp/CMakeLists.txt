cmake_minimum_required(VERSION 2.8)
#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

add_definitions(
	"-W"
	"-Wall"
	"-Werror"
	"-Wshadow"
	"-Wformat"
	"-Wpointer-arith"
	"-D_REENTRANT"
	"-D_USE_FAST_MACRO"
	"-Wno-long-long"
	"-Wuninitialized"
	"-D_POSIX_PTHREAD_SEMANTICS"
	"-DHAS_MYSQL_DLL"
	"-DHAS_SQLITE_DLL"
	"-DHAS_ZLIB_DLL"
	"-Wno-invalid-source-encoding"
	"-Wno-unused-private-field"
	"-Wno-unused-parameter"
	"-Wno-unused-const-variable"
	"-fexceptions"
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

#string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
#if (CMAKE_SYSTEM_NAME MATCHES "LINUX")
#	add_definitions("-DLINUX2")
#elseif(CMAKE_SYSTEM_NAME MATCHES "ANDROID")
#	add_definitions("-DACL_ARM_LINUX")
#else()
#	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
#endif()

##############################################################################

set(rpath lib_acl_cpp)
set(acl_path ${PROJECT_SOURCE_DIR}/lib_acl)
set(acl_include ${acl_path}/include)
set(proto_path ${PROJECT_SOURCE_DIR}/lib_protocol)
set(proto_include ${proto_path}/include)
set(mysql_include ${PROJECT_SOURCE_DIR}/include/mysql)
set(sqlite_include ${PROJECT_SOURCE_DIR}/include/sqlite)

include_directories(
	${mysql_include}
	${sqlite_include}
	${acl_include}
	${proto_include}
	${PROJECT_SOURCE_DIR}/${rpath}/src
	${PROJECT_SOURCE_DIR}/${rpath}/include
)

set(src ${PROJECT_SOURCE_DIR}/${rpath}/src)
set(sources
	${src}
	${src}/aliyun
	${src}/beanstalk
	${src}/connpool
	${src}/db
	${src}/disque
	${src}/event
	${src}/hsocket
	${src}/http
	${src}/ipc
	${src}/master
	${src}/memcache
	${src}/mime
	${src}/mime/internal
	${src}/queue
	${src}/redis
	${src}/serialize
	${src}/session
	${src}/smtp
	${src}/stdlib
	${src}/stream lib_src
)

foreach(iter ${sources})
	aux_source_directory(${iter} lib_src)
endforeach()

set(lib_output_path ${PROJECT_BINARY_DIR}/lib)

set(LIBRARY_OUTPUT_PATH ${lib_output_path})
add_library(_acl_cpp_static STATIC ${lib_src})
SET_TARGET_PROPERTIES(_acl_cpp_static PROPERTIES OUTPUT_NAME "_acl_cpp")

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-rpath,. -L${lib_output_path} -l_protocol -l_acl")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})
add_library(_acl_cpp SHARED ${lib_src})
link_directories(${lib_output_path})
target_link_libraries(_acl_cpp _protocol _acl)

##############################################################################
