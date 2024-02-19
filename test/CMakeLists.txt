set(TEST_TARGETS
    test_mm_api
    test_calculator
    test_cfgparser
    test_metersim
    test_devices
)

include_directories("../src/metersim")

foreach(target ${TEST_TARGETS})
    add_executable(${target} ${target}.c)
    target_link_libraries(${target} semsim unity)
endforeach(target)
