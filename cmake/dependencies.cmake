# This file handles downloading and extracting of prebuilt binaries

include (FetchContent)
set(FetchContent_QUIET OFF)
set(FetchContent_UPDATES_DISCONNECTED ON)
set(FetchContent_GIT_PROGRESS ON)

# Only works for windows, as of now.
if (WIN32)
	# set(PKG_CONFIG_EXECUTABLE "${VCPKG_INSTALLED_DIR}/x64-windows/tools/pkgconf/pkgconf.exe")
	# find_package(PkgConfig REQUIRED)
	# CMake >= 3.1, default search paths of cmake such as CMAKE_PREFIX_PATHs, CMAKE_FRAMEWORK_PATH,
	# and CMAKE_APPBUNDLE_PATH cache and environment variables will be added to pkg-config search path.
	# https://cmake.org/cmake/help/v3.2/module/FindPkgConfig.html

	#######################################
	##		FFMPEG 7.1
	#######################################

	# FetchContent_Declare(FFMPEG
	# 	SOURCE_DIR "${CMAKE_SOURCE_DIR}/_deps/ffmpeg_v7.1"
	# 	URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n7.1-latest-win64-gpl-shared-7.1.zip"
	# 	FIND_PACKAGE_ARGS
	# )
	# FetchContent_MakeAvailable(FFMPEG)
	# FetchContent_GetProperties(FFMPEG)

	# set(PKG_CONFIG_PATH ${FFMPEG_SOURCE_DIR}/lib/pkgconfig)
	# pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil)

	######################################

	######################################
	##		ONNXRuntime	v5.6 (Intel)
	######################################

	FetchContent_Declare(ort
		# SOURCE_DIR "${CMAKE_SOURCE_DIR}/_deps/onnxruntime_v5.6"
		URL "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/onnxruntime-win-x64-1.21.1-cpu-ov.zip"
		FIND_PACKAGE_ARGS NAMES onnxruntime
	)
    FetchContent_MakeAvailable(ort)
	FetchContent_GetProperties(ort)

	# If ORT was found and FetchContent ignores download, don't look in the download paths
	# link directly instead
	if (ort_POPULATED)
		set(onnxruntime_DIR ${ort_SOURCE_DIR}/lib/cmake/onnxruntime)
		find_package(onnxruntime CONFIG REQUIRED)
	endif()
	target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE onnxruntime::onnxruntime)

	######################################

	######################################
	##		OpenVINO 2025.1 (CPU, GPU)
	######################################
	# We're not using OpenVINO itself, but onnxruntime requires its dlls for OpenVINO EP.

	FetchContent_Declare(openvino
		SOURCE_DIR "${CMAKE_SOURCE_DIR}/_deps/openvino_2025.1"
		URL "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/openvino-2025.1.0-win-x64-auto-cpu-gpu-.zip"
	)
    FetchContent_MakeAvailable(openvino)
	FetchContent_GetProperties(openvino)

	######################################


	########################################################
	###		COPY ALL THE FILES
	########################################################

	# Copy the dlls over to the binary dir
	# NOTE: libx264-164.dll used to get copied to the binary dir as of ffmpeg v7.1 but now it doesn't
	#		https://github.com/microsoft/vcpkg/issues/43802
	file(GLOB_RECURSE	ORT_DLLS "${ort_SOURCE_DIR}/*.dll")
	file(GLOB_RECURSE	OPENVINO_DLLS "${openvino_SOURCE_DIR}/runtime/*.dll")
	file(GLOB			OPENVINO_JSON LIST_DIRECTORIES FALSE "${openvino_SOURCE_DIR}/runtime/bin/intel64/Release/*.json")
	file(GLOB			x264_DLLS LIST_DIRECTORIES FALSE "${VCPKG_INSTALLED_DIR}/x64-windows/tools/x264/bin/*.dll")

	add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		    ${ORT_DLLS}
			${OPENVINO_DLLS}
			${OPENVINO_JSON}
			${x264_DLLS}
			"${CMAKE_BINARY_DIR}"

		VERBATIM
	)

    # Copy the .onnx models over from the scripts directory to the <bin_dir>/models.
	file(GLOB MODEL_FILES LIST_DIRECTORIES FALSE "${CMAKE_SOURCE_DIR}/scripts/*.onnx")
	set(MODELS_BIN_DST "${CMAKE_BINARY_DIR}/models")

	if (MODEL_FILES)
		add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory ${MODELS_BIN_DST}

			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			    ${MODEL_FILES}
				${MODELS_BIN_DST}

			VERBATIM
		)
    endif()


	########################################################
	########################################################

endif()
