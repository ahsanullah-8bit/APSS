
cmake_minimum_required(VERSION 3.21.1)
cmake_policy(SET CMP0135 NEW)

option(LINK_INSIGHT "Link Qt Insight Tracker library" ON)
option(BUILD_QDS_COMPONENTS "Build design studio components" ON)
option(APSS_SUPPORT_CUDA_EP "Use CUDA EP for ONNXRuntime inference" OFF)
option(APSS_SUPPORT_OPENVINO_EP "Use OpenVINO EP for ONNXRuntime inference" ON)

project(APSSApp VERSION 0.1.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
enable_testing()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(PKG_CONFIG_EXECUTABLE "${VCPKG_INSTALLED_DIR}/x64-windows/tools/pkgconf/pkgconf.exe")

include(cmake/helpers.cmake)
apss_init_dependencies()

find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Qml Quick Widgets Multimedia)
find_package(onnxruntime CONFIG REQUIRED)
find_package(OpenVINO CONFIG REQUIRED)
find_package(OpenCV 4.10.0 CONFIG REQUIRED)
find_package(TBB 2022.0.0 CONFIG REQUIRED)
find_package(Eigen3 3.4.0 CONFIG REQUIRED)
find_package(reflectcpp CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libavdevice libavfilter libswresample libswscale)
pkg_check_modules(ODB REQUIRED libodb libodb-qt libodb-sqlite)
qt_standard_project_setup()

# Qt Policies
if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)
set(QML_IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    CACHE STRING "Import paths for Qt Creator's code model"
    FORCE
)

qt_add_executable(${CMAKE_PROJECT_NAME})
qt_add_resources(${CMAKE_PROJECT_NAME} "configuration"
    PREFIX "/"
    FILES
    qtquickcontrols2.conf)

apss_copy_runtime_files()

add_subdirectory(ext)
include(qds)

if (BUILD_QDS_COMPONENTS)
    include(qmlcomponents OPTIONAL)
endif()

if (LINK_INSIGHT)
    include(insight OPTIONAL)
endif ()

include(GNUInstallDirs)
install(TARGETS ${CMAKE_PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
  $<$<BOOL:${APSS_SUPPORT_CUDA_EP}>:APSS_SUPPORT_CUDA_EP>
  $<$<BOOL:${APSS_SUPPORT_OPENVINO_EP}>:APSS_SUPPORT_OPENVINO_EP>
)

add_subdirectory(test)
