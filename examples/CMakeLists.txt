add_executable(simulate_metersim simulate_metersim/simulate_metersim.c)
add_executable(simulate_mm_api simulate_mm_api/simulate_mm_api.c)

target_link_libraries(simulate_metersim semsim)
target_link_libraries(simulate_mm_api semsim)
