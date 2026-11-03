cmake_minimum_required(VERSION 3.24)

set(CAONGAN_PACKAGE_VERSION "" CACHE STRING "Package version")
set(CAONGAN_APP_SOURCE_DIR "" CACHE PATH "Source tree to package")
if(NOT CAONGAN_PACKAGE_VERSION MATCHES "^(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)$")
    message(FATAL_ERROR "CAONGAN_PACKAGE_VERSION must be MAJOR.MINOR.PATCH")
endif()
if(NOT EXISTS "${CAONGAN_APP_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "CAONGAN_APP_SOURCE_DIR must point to the source checkout")
endif()
project(caongan_led_package VERSION ${CAONGAN_PACKAGE_VERSION} LANGUAGES CXX)
if(NOT CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
    message(FATAL_ERROR "This DEB targets amd64")
endif()
include(GNUInstallDirs)
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 QuickLayouts Sql)

# Đóng gói source ở commit được chọn, kể cả tag cũ chưa có mã đóng gói.
# EXCLUDE_FROM_ALL ngăn install rule cũ đặt executable ở sai vị trí.
set(CAONGAN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CAONGAN_BUILD_VERSION "${CAONGAN_PACKAGE_VERSION}" CACHE STRING "" FORCE)
add_subdirectory("${CAONGAN_APP_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/application" EXCLUDE_FROM_ALL)

install(TARGETS caongan_led RUNTIME DESTINATION bin)
qt_generate_deploy_qml_app_script(
    TARGET caongan_led
    OUTPUT_SCRIPT caongan_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    NO_TRANSLATIONS
    INCLUDE_PLUGINS qoffscreen qwayland
)
install(SCRIPT ${caongan_deploy_script})

install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/caongan-led" DESTINATION /usr/bin)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/caongan-led.desktop" DESTINATION /usr/share/applications)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/caongan-led.svg"
    DESTINATION /usr/share/icons/hicolor/scalable/apps)
install(FILES "${CAONGAN_APP_SOURCE_DIR}/README.md" "${CAONGAN_APP_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
    DESTINATION /usr/share/doc/caongan-led)
install(FILES "${CAONGAN_APP_SOURCE_DIR}/shared/logger-ui-kit/LICENSE"
    DESTINATION /usr/share/doc/caongan-led RENAME logger-ui-kit-MIT.txt)
install(FILES "${CAONGAN_APP_SOURCE_DIR}/resources/fonts/Inter/OFL.txt"
    DESTINATION /usr/share/doc/caongan-led RENAME Inter-OFL.txt)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME caongan-led)
set(CPACK_PACKAGE_VENDOR "Cao Ngạn LED")
set(CPACK_PACKAGE_CONTACT "Cao Ngạn LED maintainers")
set(CPACK_PACKAGE_VERSION "${CAONGAN_PACKAGE_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "caongan-led_${CAONGAN_PACKAGE_VERSION}_amd64")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cao Ngạn LED environmental display")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
set(CPACK_DEBIAN_PACKAGE_SECTION utils)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
# Qt/QML được deploy riêng; đây là các thư viện hệ thống bắt buộc trên Ubuntu 24.04.
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libc6 (>= 2.39), libstdc++6, libgcc-s1, libatomic1, libzstd1, libxkbcommon0, libxkbcommon-x11-0, libegl1, libgl1, libopengl0, libfontconfig1, libdbus-1-3, libgssapi-krb5-2, libglib2.0-0 | libglib2.0-0t64, libxcb1, libx11-6, libx11-xcb1, libxcb-cursor0, libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-randr0, libxcb-render0, libxcb-render-util0, libxcb-shape0, libxcb-shm0, libxcb-sync1, libxcb-util1, libxcb-xfixes0, libxcb-xkb1, libwayland-client0, libwayland-cursor0, libwayland-egl1"
)
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/caongan-led")
include(CPack)
