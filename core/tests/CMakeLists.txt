find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(Catch2)
endif()

add_executable(volta_core_tests
    test_dense_lu.cpp
    test_ac_analysis.cpp
    test_frequency_response.cpp
    test_radio.cpp
    test_radio_module.cpp
    test_fm_module.cpp
    test_second_order.cpp
    test_events.cpp
    test_reference_dc.cpp
    test_robustness.cpp
    test_realtime.cpp
    components/test_resistor.cpp
    components/test_battery.cpp
    components/test_diode.cpp
    components/test_capacitor.cpp
    test_thermal.cpp
    test_blink_module.cpp
    test_tone_module.cpp
    test_timer_amplifier.cpp
    test_opamp_module.cpp
    test_timer555.cpp
    test_gate_module.cpp
    test_counter_module.cpp
    test_signal_stats.cpp
    test_fuzz.cpp
    components/test_transistor.cpp
    components/test_lamp_motor.cpp
    components/test_switch_buzzer.cpp
    components/test_meter.cpp
    components/test_potentiometer.cpp
    components/test_speaker.cpp
    components/test_sensors_magnet.cpp
    components/test_optics.cpp
    components/test_regulator.cpp
    components/test_sources.cpp
    components/test_inductor.cpp
    components/test_transformer.cpp
    components/test_thyristor.cpp
    components/test_mosfet.cpp
)
target_include_directories(volta_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(volta_core_tests PRIVATE volta::core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(volta_core_tests)
