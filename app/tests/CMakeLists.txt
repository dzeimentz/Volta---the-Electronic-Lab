# Tests fuer Schaltungsmodell und Dienste (Qt Test, docs/testing.md)
find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(volta_model_tests test_model.cpp)
target_link_libraries(volta_model_tests PRIVATE volta_model Qt6::Test)
target_compile_definitions(volta_model_tests PRIVATE VOLTA_CONTENT_DIR="${PROJECT_SOURCE_DIR}/content")
add_test(NAME volta_model_tests COMMAND volta_model_tests)

# Qt-Bibliotheken fuer den Testlauf auffindbar machen (Windows: PATH)
if(WIN32)
    get_target_property(_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qtbin "${_qmake}" DIRECTORY)
    set_tests_properties(volta_model_tests PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qtbin}")
endif()
