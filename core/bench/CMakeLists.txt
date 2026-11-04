# Leistungsmessung des Simulationskerns (docs/testing.md). Aussagekraeftig nur im Release-Build:
#   cmake --preset core-release && cmake --build --preset core-release && build/core-release/core/bench/volta_core_bench
add_executable(volta_core_bench bench_main.cpp)
target_link_libraries(volta_core_bench PRIVATE volta::core)

# Leistungstest Volta 2 (hoerbare Simulation, 20-us-Schritte): build/core-release/core/bench/volta_core_bench_audio
add_executable(volta_core_bench_audio bench_audio.cpp)
target_link_libraries(volta_core_bench_audio PRIVATE volta::core)

# Machbarkeitstest Volta 3 (Frequenzgang, Radio, Schaltwandler): build/core-release/core/bench/volta_core_bench_volta3
add_executable(volta_core_bench_volta3 bench_volta3.cpp)
target_link_libraries(volta_core_bench_volta3 PRIVATE volta::core)
