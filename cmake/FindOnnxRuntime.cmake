# FindOnnxRuntime.cmake responsible for ONNXRuntime

# ONNXRuntime
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(onnxruntime
    DOWNLOAD_EXTRACT_TIMESTAMP true
    # Keep this URL last or we get a surprise (source: https://stackoverflow.com/questions/74996365/cmake-error-at-least-one-entry-of-url-is-a-path-invalid-in-a-list?)
    URL "https://github.com/microsoft/onnxruntime/releases/download/v1.21.0/onnxruntime-win-x64-1.21.0.zip"
)
FetchContent_MakeAvailable(onnxruntime)
FetchContent_GetProperties(onnxruntime)

find_path(OnnxRuntime_INCLUDE_DIR
    onnxruntime_cxx_api.h
    HINTS "${onnxruntime_SOURCE_DIR}/include")
find_library(OnnxRuntime_LIBRARY
    NAMES onnxruntime
    HINTS "${onnxruntime_SOURCE_DIR}/lib")
find_library(OnnxRuntime_PROVIDERS_SHARED_LIBRARY
    NAMES onnxruntime_providers_shared
    HINTS "${onnxruntime_SOURCE_DIR}/lib")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OnnxRuntime REQUIRED_VARS
    OnnxRuntime_INCLUDE_DIR
    OnnxRuntime_LIBRARY
    OnnxRuntime_PROVIDERS_SHARED_LIBRARY

    VERSION_VAR 1.21.0
)

if (OnnxRuntime_FOUND)
    set(OnnxRuntime_LIBS ${OnnxRuntime_LIBRARY} ${OnnxRuntime_PROVIDERS_SHARED_LIBRARY})
    set(OnnxRuntime_INCLUDE_DIRS ${OnnxRuntime_INCLUDE_DIR})
    set(OnnxRuntime_LIB_DIR "${onnxruntime_SOURCE_DIR}/lib")

    # Copy over the dlls and other files, to the binary directory.
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${OnnxRuntime_LIB_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}

        RESULT_VARIABLE ONNX_COPY_VAR
    )

endif()
