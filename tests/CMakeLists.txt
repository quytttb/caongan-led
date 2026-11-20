find_package(Qt6 6.11 REQUIRED COMPONENTS Test)

function(caongan_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE
        Qt6::Test
        caongan_domain
        caongan_storage
        caongan_ingestion
        caongan_utils
    )
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;CAONGAN_LED_BASE_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-data"
    )
endfunction()

caongan_add_test(tst_tt10)
caongan_add_test(tst_config)
caongan_add_test(tst_snapshot)
caongan_add_test(tst_scan)
caongan_add_test(tst_reading_store)
caongan_add_test(tst_mapping)
caongan_add_test(tst_ingestion)
caongan_add_test(tst_presentation)
target_link_libraries(tst_presentation PRIVATE caongan_presentation Qt6::Gui)
caongan_add_test(tst_log_ui)
target_link_libraries(tst_log_ui PRIVATE caongan_presentation Qt6::Gui Qt6::Qml Qt6::Quick
    LoggerKit::theme logger_ui_kit_themeplugin)
target_compile_definitions(tst_log_ui PRIVATE
    CAONGAN_QML_SOURCE_DIR="${PROJECT_SOURCE_DIR}/src/app/qml")
caongan_add_test(tst_window_ui)
target_link_libraries(tst_window_ui PRIVATE caongan_presentation Qt6::Gui Qt6::Qml Qt6::Quick
    Qt6::QuickControls2 LoggerKit::theme logger_ui_kit_themeplugin)
target_compile_definitions(tst_window_ui PRIVATE
    CAONGAN_QML_SOURCE_DIR="${PROJECT_SOURCE_DIR}/src/app/qml")
qt_add_resources(tst_window_ui "test_brand"
    PREFIX "/branding"
    BASE "${PROJECT_SOURCE_DIR}/resources"
    FILES "${PROJECT_SOURCE_DIR}/resources/factory-logo.jpg")
qt_add_resources(tst_window_ui "test_fonts"
    PREFIX "/fonts"
    BASE "${PROJECT_SOURCE_DIR}/resources/fonts"
    FILES
        "${PROJECT_SOURCE_DIR}/resources/fonts/Inter/Inter-Regular.otf"
        "${PROJECT_SOURCE_DIR}/resources/fonts/Inter/Inter-Bold.otf")
set_tests_properties(tst_window_ui PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen:configfile=offscreen-screens.json;QT_QUICK_CONTROLS_STYLE=Material")
set_tests_properties(tst_presentation PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen:configfile=offscreen-screens.json;CAONGAN_LED_BASE_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-data"
)

add_test(NAME tst_app_version COMMAND $<TARGET_FILE:caongan_led> --version)
string(REPLACE "." "\\." CAONGAN_TEST_VERSION_PATTERN "${PROJECT_VERSION}")
set_tests_properties(tst_app_version PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    PASS_REGULAR_EXPRESSION "CaonganLed ${CAONGAN_TEST_VERSION_PATTERN}"
)
