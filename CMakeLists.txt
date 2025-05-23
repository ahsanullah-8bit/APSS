
cmake_minimum_required(VERSION 3.21.1)
cmake_policy(SET CMP0135 NEW)

option(LINK_INSIGHT "Link Qt Insight Tracker library" ON)
option(BUILD_QDS_COMPONENTS "Build design studio components" ON)
option(APSS_USE_CUDA_EP "Use onnxruntime CUDA for inference" OFF)
option(APSS_USE_OPENVINO_EP "Use OpenVINO EP for onnxruntime for inference" ON)

project(APSSApp VERSION 0.1.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)
set(QML_IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    CACHE STRING "Import paths for Qt Creator's code model"
    FORCE
)

# Packages
# The ones provided by vcpkg are here and linked in App/CMakeLists.txt
# The ones fetched as prebuilt are handled by cmake/dependencies.cmake
# The submodules are managed at Dependencies/CMakeLists.txt
find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Qml Quick Multimedia)
find_package(OpenCV 4.10.0 CONFIG REQUIRED)
find_package(TBB 2022.0.0 CONFIG REQUIRED)
set(PKG_CONFIG_EXECUTABLE "${VCPKG_INSTALLED_DIR}/x64-windows/tools/pkgconf/pkgconf.exe")
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libavdevice libavfilter libswresample libswscale)
qt_standard_project_setup()

# Qt Policies
if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

# Confirm FFMPEG
# message(STATUS "FFMPEG_INCLUDE_DIRS: ${FFMPEG_INCLUDE_DIRS}")
# message(STATUS "FFMPEG_LIBRARY_DIRS: ${FFMPEG_LIBRARY_DIRS}")
# message(STATUS "FFMPEG_LIBRARIES: ${FFMPEG_LIBRARIES}")

# Export SCRIPTS_DIR, for the weights to be found.
set(SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/scripts")
configure_file(exports.h.in exports.h @ONLY)

qt_add_executable(${CMAKE_PROJECT_NAME})
qt_add_resources(${CMAKE_PROJECT_NAME} "configuration"
    PREFIX "/"
    FILES
    qtquickcontrols2.conf)

include(cmake/dependencies.cmake)
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

if (APSS_USE_CUDA_EP)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE APSS_USE_CUDA_EP)
elseif (APSS_USE_OPENVINO_EP)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE APSS_USE_OPENVINO_EP)
else()
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE APSS_USE_CPU_EP)
endif()
