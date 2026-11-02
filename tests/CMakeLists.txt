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
set_tests_properties(tst_presentation PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen:configfile=offscreen-screens.json;CAONGAN_LED_BASE_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-data"
)
