cmake_minimum_required(VERSION 3.1.3)
project(lock-free-parallel-hashing CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
#  -ggdb -fno-omit-frame-pointer -fno-rtti -fno-exceptions -fno-inline
add_compile_options(-O3 -Wall -Wextra -Wno-unused-function -march=native -mtune=native)


link_libraries("-lpthread")

string ( REPLACE "/" "\/" remove_path_regex ${CMAKE_CURRENT_SOURCE_DIR} + "/")

file(GLOB DEP_SOURCES dep/*.cc)
file(GLOB EXE_SOURCES src/*.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(lib)

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/lib/*.cc")



foreach(DEP_SRC_CODE ${DEP_SOURCES})
  STRING( REPLACE "dep/" "" _dep1 ${DEP_SRC_CODE})
  STRING( REPLACE ".cc" "" _dep2 ${_dep1})
  string( REGEX REPLACE ${remove_path_regex} "" DEP ${_dep2} )
  add_executable(${DEP} ${DEP_SRC_CODE} ${SOURCES})

  set(DEP_CMD "run_${DEP}")
  add_custom_target(${DEP_CMD}
    COMMAND ${DEP}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES ${DEP_SRC_CODE}
    COMMENT "Running: ${DEP}"
    )
  list(APPEND DEP_COMMANDS ${DEP_CMD})
endforeach(DEP_SRC_CODE ${DEP_SOURCES})


foreach(EXE_SRC_CODE ${EXE_SOURCES})
  STRING( REPLACE "src/" "" _exe1 ${EXE_SRC_CODE})
  STRING( REPLACE ".cc" "" _exe2 ${_exe1})
  string( REGEX REPLACE ${remove_path_regex} "" EXE ${_exe2} )
  add_executable(${EXE} ${EXE_SRC_CODE} ${SOURCES})
  target_link_libraries(${EXE})
  foreach(BUILD_DEP ${DEP_COMMANDS})
    add_dependencies(${EXE} ${BUILD_DEP})
  endforeach(BUILD_DEP ${DEP_COMMANDS})
endforeach(EXE_SRC_CODE ${EXE_SOURCES})


