cmake_minimum_required(VERSION 3.5)
project(Smart_Energy_Meter_Simulator)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-Wall -Wextra")

include_directories(include)

if (CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_definitions(LOG_LVL=1)
else()
    add_compile_definitions(LOG_LVL=4)
endif()

set(SRC_FILES
    src/metersim/cfgparser.h
    src/metersim/cfgparser.c
    src/metersim/runner.c
    src/metersim/metersim.c
    src/metersim/simulator.c
    src/metersim/time_machine.h
    src/metersim/metersim_types_int.h
    src/metersim/time_machine.c
    src/metersim/calculator.h
    src/metersim/simulator.h
    src/metersim/log.h
    src/metersim/runner.h
    src/metersim/calculator.c
    src/metersim/devicemgr.h
    src/metersim/devicemgr.c

    src/mm_api/api_host.c
)

set(PY_FILES
    src/python_utils/complex_py.h
    src/python_utils/complex_py.c
    src/python_utils/device_py.c
)

add_library(semsim STATIC ${SRC_FILES})
add_library(semsim_py SHARED ${SRC_FILES} ${PY_FILES})


# Include Unity for unit testing
add_compile_definitions(UNITY_INCLUDE_DOUBLE)
add_subdirectory(external/Unity EXCLUDE_FROM_ALL)
include_directories(external/Unity/src)


# Include tomlc99 for parsing .toml files
include_directories(external/tomlc99)
add_custom_target(
    libtomlc99
    COMMAND make install prefix=${CMAKE_BINARY_DIR}/external/tomlc99
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/tomlc99
)

add_dependencies(semsim libtomlc99)
add_dependencies(semsim_py libtomlc99)

target_link_libraries(semsim m ${CMAKE_BINARY_DIR}/external/tomlc99/lib/libtoml.a)
target_link_libraries(semsim_py m ${CMAKE_BINARY_DIR}/external/tomlc99/lib/libtoml.a)

add_subdirectory(test)
add_subdirectory(examples)
