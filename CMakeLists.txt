cmake_minimum_required(VERSION 3.24)

project(caongan_led
    VERSION 0.1.0
    DESCRIPTION "Màn LED quan trắc khí thải và nước thải Cao Ngạn"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.11 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    QuickLayouts
    Sql
)

qt_standard_project_setup(REQUIRES 6.11)
qt_policy(SET QTP0004 NEW)

if(MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Static QML modules phải nằm đúng cây <build>/<URI> để qmlimportscanner tìm thấy
# qmldir và đăng ký plugin tĩnh vào executable. Đặt trước mọi qt_add_qml_module.
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# UI kit: chỉ theme (không kéo LoggerKit.Components/QtGraphs). Revision được pin
# trong shared/logger-ui-kit/.git và ghi lại ở docs/provenance.md.
set(LOGGERKIT_QML_MODULE_VERSION 1.0)
add_subdirectory(shared/logger-ui-kit/src/theme)

# ---------------------------------------------------------------------------
# Domain library: parser, thời gian, catalog, trạng thái, ngưỡng, config.
# ---------------------------------------------------------------------------
add_library(caongan_domain STATIC
    src/domain/Types.cpp
    src/domain/Tt10Time.cpp
    src/domain/ParameterCatalog.cpp
    src/domain/ConfigCodec.cpp
    src/domain/Tt10Parser.cpp
    src/domain/ParameterMapper.cpp
    src/domain/LatestReadingStore.cpp
    src/domain/Freshness.cpp
    src/domain/ThresholdEvaluator.cpp
    src/domain/DisplayState.cpp
)
target_include_directories(caongan_domain PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(caongan_domain PUBLIC Qt6::Core)

# ---------------------------------------------------------------------------
# Utils: đường dẫn ứng dụng (không phụ thuộc CWD) + logging xoay vòng.
# ---------------------------------------------------------------------------
add_library(caongan_utils STATIC
    src/utils/AppPaths.cpp
    src/utils/LogSetup.cpp
)
target_link_libraries(caongan_utils PUBLIC Qt6::Core)

# ---------------------------------------------------------------------------
# Storage: JSON config atomic + SQLite snapshot/checkpoint.
# ---------------------------------------------------------------------------
add_library(caongan_storage STATIC
    src/storage/ConfigStore.cpp
    src/storage/SnapshotStore.cpp
)
target_link_libraries(caongan_storage PUBLIC
    caongan_domain
    Qt6::Core
    Qt6::Sql
)

# ---------------------------------------------------------------------------
# Ingestion: quét thư mục chỉ đọc, watcher, parser/mapping, SQLite.
# ---------------------------------------------------------------------------
add_library(caongan_ingestion STATIC
    src/ingestion/DirectoryScan.cpp
    src/ingestion/IngestionService.cpp
)
target_link_libraries(caongan_ingestion PUBLIC
    caongan_storage
    Qt6::Core
)

# ---------------------------------------------------------------------------
# Presentation: model QML và controller; không quyết định nghiệp vụ trong QML.
# ---------------------------------------------------------------------------
add_library(caongan_presentation STATIC
    src/presentation/ApplicationController.cpp
    src/presentation/DiagnosticsModel.cpp
    src/presentation/LedParameterModel.cpp
    src/presentation/SimulationFactory.cpp
)
target_link_libraries(caongan_presentation PUBLIC
    caongan_ingestion
    caongan_utils
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
)

# ---------------------------------------------------------------------------
# Application + QML module
# ---------------------------------------------------------------------------
qt_add_executable(caongan_led
    src/app/main.cpp
)

qt_add_qml_module(caongan_led
    URI CaonganLed.App
    VERSION 1.0
    QML_FILES
        src/app/qml/Main.qml
        src/app/qml/LedWindow.qml
        src/app/qml/LedBoard.qml
        src/app/qml/ParameterTile.qml
        src/app/qml/LedStatusLabel.qml
        src/app/qml/OperatorWindow.qml
        src/app/qml/SourceSettingsPage.qml
        src/app/qml/ChannelSettingsPage.qml
        src/app/qml/DisplaySettingsPage.qml
        src/app/qml/DiagnosticsPage.qml
        src/app/qml/ActionDialog.qml
        src/app/qml/InlineNotice.qml
)

qt_add_resources(caongan_led "app_fonts"
    PREFIX "/fonts"
    BASE "resources/fonts"
    FILES
        resources/fonts/Inter/Inter-Regular.otf
        resources/fonts/Inter/Inter-Medium.otf
        resources/fonts/Inter/Inter-SemiBold.otf
        resources/fonts/Inter/Inter-Bold.otf
        resources/fonts/Inter/Inter-Italic.otf
        resources/fonts/Inter/Inter-BoldItalic.otf
        resources/fonts/Inter/OFL.txt
)

qt_add_resources(caongan_led "factory_brand"
    PREFIX "/branding"
    BASE "resources"
    FILES resources/factory-logo.jpg
)

target_compile_definitions(caongan_led PRIVATE
    CAONGAN_LED_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(caongan_led PRIVATE
    caongan_presentation
    caongan_domain
    caongan_storage
    caongan_ingestion
    caongan_utils
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickLayouts
    LoggerKit::theme
    LoggerKit::theme_setup
)

# Static QML module LoggerKit.Theme cần đăng ký plugin vào executable.
qt_import_qml_plugins(caongan_led)

install(TARGETS caongan_led
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)

option(CAONGAN_BUILD_TESTS "Build unit/integration tests" ON)
if(CAONGAN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
