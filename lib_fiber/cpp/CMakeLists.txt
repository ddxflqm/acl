cmake_minimum_required(VERSION 2.8)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

message(${PROJECT_SOURCE_DIR})

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

string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
if(CMAKE_SYSTEM_NAME MATCHES "ANDROID")
	add_definitions("-DANDROID")
elseif(CMAKE_SYSTEM_NAME MATCHES "LINUX")
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

##############################################################################

set(rpath lib_fiber/cpp)
set(src ${PROJECT_SOURCE_DIR}/${rpath}/src)
set(acl_path ${PROJECT_SOURCE_DIR}/lib_acl)
set(acl_include ${acl_path}/include)
set(acl_cpp_path ${PROJECT_SOURCE_DIR}/lib_acl_cpp)
set(acl_cpp_include ${acl_cpp_path}/include)
set(fiber_path ${PROJECT_SOURCE_DIR}/lib_fiber/c)
set(fiber_include ${fiber_path}/include)

include_directories(
	${PROJECT_SOURCE_DIR}/${rpath}/src
	${PROJECT_SOURCE_DIR}/${rpath}/include
	${acl_include}
	${acl_cpp_include}
	${fiber_include}
)

set(sources ${src})

foreach(iter ${sources})
	aux_source_directory(${iter} lib_src)
endforeach()

set(lib_output_path ${PROJECT_BINARY_DIR}/lib)

set(LIBRARY_OUTPUT_PATH ${lib_output_path})
add_library(_fiber_cpp_static STATIC ${lib_src})
SET_TARGET_PROPERTIES(_fiber_cpp_static PROPERTIES OUTPUT_NAME "_fiber_cpp")

set(CMAKE_SHARED_LINKER_FLAGS
	-Wl,-rpath,. -L${lib_output_path}
	-l_acl_cpp -l_protocol -l_fiber -l_acl)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})
add_library(_fiber_cpp SHARED ${lib_src})
link_directories(${lib_output_path})
target_link_libraries(_fiber_cpp _acl_cpp _protocol _fiber _acl)

##############################################################################
