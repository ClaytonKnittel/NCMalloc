cmake_minimum_required(VERSION 3.1.3)
project(lock-free-parallel-hashing CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
#  -ggdb -fno-omit-frame-pointer -fno-rtti -fno-exceptions -fno-inline
add_compile_options(-O3 -Wall -Wextra -Wno-unused-function -march=native -mtune=native)


link_libraries("-lpthread")

string ( REPLACE "/" "\/" remove_path_regex ${CMAKE_CURRENT_SOURCE_DIR} + "/")

set(GEN_SYS_HEADER_FILE ";${CMAKE_CURRENT_SOURCE_DIR}/src/gen_system_header.cc")
string ( REPLACE "/" "\/" REMOVE_GEN_SOURCE ${GEN_SYS_HEADER_FILE})


file(GLOB EXE_SOURCES_0 src/*.cc)
STRING( REGEX REPLACE "${REMOVE_GEN_SOURCE}" "" EXE_SOURCES_1 ${EXE_SOURCES_0})
STRING( REPLACE ".cc" ".cc;" EXE_SOURCES ${EXE_SOURCES_1})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(lib)

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/lib/*.cc")



add_executable(gen_sys_header ${GEN_SYS_HEADER_FILE} ${SOURCES})
target_link_libraries(gen_sys_header)
add_custom_target(run_gen_sys_header
  COMMAND gen_sys_header
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generating PRECOMPUTED_SYS_INFO.h"
  SOURCES ${GEN_SYS_HEADER_FILE}
  )

foreach(EXE_SRC_CODE ${EXE_SOURCES})
  STRING( REPLACE "src/" "" _exe1 ${EXE_SRC_CODE})
  STRING( REPLACE ".cc" "" _exe2 ${_exe1})
  string( REGEX REPLACE ${remove_path_regex} "" EXE ${_exe2} )
  add_executable(${EXE} ${EXE_SRC_CODE} ${SOURCES})
  target_link_libraries(${EXE})
  add_dependencies(${EXE} run_gen_sys_header)
endforeach(EXE_SRC_CODE ${EXE_SOURCES})

