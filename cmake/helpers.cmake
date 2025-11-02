include_guard()

include (FetchContent)
cmake_policy(SET CMP0169 OLD)
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_GIT_PROGRESS ON)

# Populates all the required dependencies, that can't be done other wise.
function(apss_init_dependencies)
	# Only works for windows, as of now.
	if (WIN32)

		######################################
		##		Models
		######################################

		FetchContent_Declare(models
			URL "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/models.zip"
			SOURCE_DIR "${CMAKE_BINARY_DIR}/models"
		)
	    FetchContent_MakeAvailable(models)

		######################
		# Prebuilt
		######################

		######################################
		##		ONNXRuntime	v5.6 (Intel)
		######################################

		if (onnxruntime_DIR AND NOT onnxruntime_DIR STREQUAL "")
			set(onnxruntime_ROOT "${onnxruntime_DIR}/../../../" CACHE STRING "Path to onnxruntime root directory")
		elseif (onnxruntime_ROOT AND NOT onnxruntime_ROOT STREQUAL "")
			set(onnxruntime_DIR "${onnxruntime_ROOT}/lib/cmake/onnxruntime" CACHE STRING "Path to onnxruntime config files")
		else()
			message(NOTICE "--- Setting up onnxruntime ---")

			FetchContent_Declare(onnxruntime
				URL "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/onnxruntime-win-x64-v5.6-cpu-ov.zip"
			)
		    FetchContent_MakeAvailable(onnxruntime)
			FetchContent_GetProperties(onnxruntime)

			if (onnxruntime_POPULATED)
				set(onnxruntime_DIR "${onnxruntime_SOURCE_DIR}/lib/cmake/onnxruntime" CACHE STRING "Path to onnxruntime config files")
				set(onnxruntime_ROOT ${onnxruntime_SOURCE_DIR} CACHE STRING "Path to onnxruntime root directory")
			endif()

			message(NOTICE "--- Setup onnxruntime completed ---")
		endif()

		######################################
		##		OpenVINO 2025.1 (CPU, GPU)
		######################################
		# We're not using OpenVINO itself, but onnxruntime requires its dlls for OpenVINO EP.

		if (OpenVINO_DIR AND NOT OpenVINO_DIR STREQUAL "")
			set(OpenVINO_ROOT "${OpenVINO_DIR}/../../" CACHE STRING "Path to OpenVINO root directory")
		elseif(OpenVINO_ROOT AND NOT OpenVINO_ROOT STREQUAL "")
			set(OpenVINO_DIR "${OpenVINO_ROOT}/runtime/cmake" CACHE STRING "Path to OpenVINO config files")
		else()
			FetchContent_Declare(OpenVINO
				URL "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/openvino-2025.1.0-win-x64-auto-cpu-gpu.zip"
			)
		    FetchContent_MakeAvailable(OpenVINO)
			FetchContent_GetProperties(OpenVINO)

			if (openvino_POPULATED)
				set(OpenVINO_DIR "${openvino_SOURCE_DIR}/runtime/cmake"  CACHE STRING "Path to OpenVINO config files")
				set(OpenVINO_ROOT ${openvino_SOURCE_DIR} CACHE STRING "Path to OpenVINO root directory")
			endif()
		endif()

		######################################
		##			odb 2.5.0
		######################################

		# odb.exe
		if (NOT odb_EXECUTABLE OR odb_EXECUTABLE STREQUAL "")
			set(ODB_EXE_ARCHIVE "https://www.codesynthesis.com/download/odb/2.5.0/windows/windows10/x86_64/odb-2.5.0-x86_64-windows10.zip")

			FetchContent_Declare(odbexe
				URL ${ODB_EXE_ARCHIVE}
			)
		    FetchContent_MakeAvailable(odbexe)
			FetchContent_GetProperties(odbexe)

			set(odb_EXECUTABLE "${odbexe_SOURCE_DIR}/bin/odb.exe" CACHE STRING "Path to the odb.exe")
		endif()

		# libodb
		if (libodb_ROOT AND NOT libodb_ROOT STREQUAL "")
			list(APPEND CMAKE_PREFIX_PATH ${libodb_ROOT})
			set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE) # Propagate the variable
		else()
			set(ODB_ARCHIVE "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/libodb-2.5.0-release.zip")
			set(ODB_URL_HASH "SHA256=cf2f1ce16ac68d146dc211e1fed3d2ad830f6daa677f664407e70c318a3741d4")
			if (CMAKE_BUILD_TYPE STREQUAL "Debug")
				set(ODB_ARCHIVE "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/libodb-2.5.0-debug.zip")
				set(ODB_URL_HASH "SHA256=e55421cf7f2b9e817187f22af7574eeced72866d7b2e863e8b99153c6a883f3b")
			endif()

			# libodb
			FetchContent_Declare(libodb
				URL ${ODB_ARCHIVE}
				URL_HASH ${ODB_URL_HASH}
			)
		    FetchContent_MakeAvailable(libodb)
			FetchContent_GetProperties(libodb)

			set(libodb_ROOT ${libodb_SOURCE_DIR} CACHE STRING "Path to libodb root directory")

			list(APPEND CMAKE_PREFIX_PATH ${libodb_SOURCE_DIR})
			set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE) # Propagate the variable
		endif()

		# Change the \\ in every .pc files to /, as pkgconf is reading \\..\\.. as .... from the .pc files..
		apss_patch_odb_pc_files(${libodb_ROOT})

		######################################

		# Debug
		# message(STATUS "onnxruntime_ROOT: ${onnxruntime_DIR}")
		# message(STATUS "OpenVINO_ROOT: ${OpenVINO_ROOT}")
		# message(STATUS "odb_EXECUTABLE: ${odb_EXECUTABLE}")
		# message(STATUS "libodb_ROOT: ${libodb_ROOT}")

		######################

		# WARNING: These checks are necessary, as the GLOB_RECURSE will search the WHOLE SYSTEM, if the <globbing-expressions> is given as "/*.dll".
		if (onnxruntime_ROOT AND NOT onnxruntime_ROOT STREQUAL "")
			file(GLOB_RECURSE	ORT_DLLS			LIST_DIRECTORIES FALSE "${onnxruntime_ROOT}/*.dll") # <dir>/lib/cmake/onnxruntime
		endif()

		if (OpenVINO_ROOT AND NOT OpenVINO_ROOT STREQUAL "")
			file(GLOB_RECURSE	OPENVINO_DLLS		LIST_DIRECTORIES FALSE "${OpenVINO_ROOT}/runtime/*.dll")
			file(GLOB			OPENVINO_JSON		LIST_DIRECTORIES FALSE "${OpenVINO_ROOT}/runtime/bin/intel64/Release/*.json")
		endif()

		# NOTE: libx264-164.dll used to get copied to the binary dir as of ffmpeg v7.1 but now it doesn't
		#		https://github.com/microsoft/vcpkg/issues/43802
		if (VCPKG_INSTALLED_DIR AND NOT ${VCPKG_INSTALLED_DIR} STREQUAL "")
			file(GLOB			x264_DLLS			LIST_DIRECTORIES FALSE "${VCPKG_INSTALLED_DIR}/x64-windows/tools/x264/bin/*.dll")
		endif()

		if (libodb_ROOT AND NOT libodb_ROOT STREQUAL "")
			file(GLOB			ODB_FILES			LIST_DIRECTORIES FALSE "${libodb_ROOT}/bin/*.dll")
		endif()

		# Add any list of dlls that you want to get copied to the binary directory
	    set(temp_postbuild_runtime_files
			${ORT_DLLS}
			${OPENVINO_DLLS}
			${OPENVINO_JSON}
			${x264_DLLS}
			${ODB_FILES}
		)

	    # This makes 'APSS_TARGET_RUNTIME_FILES' available in the scope that called this function.
		set(APSS_PREBUILD_FILES ${APSS_PREBUILD_FILES} ${temp_prebuild_files} PARENT_SCOPE)
		set(APSS_POSTBUILD_RUNTIME_FILES ${APSS_POSTBUILD_RUNTIME_FILES} ${temp_postbuild_runtime_files} PARENT_SCOPE)

	endif() # WIN32

endfunction() # apss_init_dependencies

# Copies all the necessary files like dlls, models, etc. over to the binary dir. So, they are all available at runtime
function(apss_copy_runtime_files)
	# Copy the prebuild files over to the binary dir
	if (APSS_PREBUILD_FILES AND NOT APSS_PREBUILD_FILES STREQUAL "")

		message(STATUS "--- Listing prebuild files (is being copied): ---")
		foreach(file_path IN LISTS APSS_PREBUILD_FILES)
			message(STATUS "  - ${file_path}")
		endforeach()
		message(STATUS "--- End of prebuild files list ---")

		add_custom_command(TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${APSS_PREBUILD_FILES}
			${CMAKE_BINARY_DIR}

			VERBATIM
		)
    endif()

	# Copy the dlls over to the binary dir
	if (APSS_POSTBUILD_RUNTIME_FILES AND NOT APSS_POSTBUILD_RUNTIME_FILES STREQUAL "")

		message(STATUS "--- Listing runtime files (will be copied): ---")
		foreach(file_path IN LISTS APSS_POSTBUILD_RUNTIME_FILES)
			message(STATUS "  - ${file_path}")
		endforeach()
		message(STATUS "--- End of runtime files list ---")

		add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			    ${APSS_POSTBUILD_RUNTIME_FILES}
				${CMAKE_BINARY_DIR}

			VERBATIM
		)
    endif()

	# Copy the .onnx models over from the scripts directory to the <bin_dir>/models.
	set(SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/scripts")
	file(GLOB MODEL_FILES LIST_DIRECTORIES FALSE "${SCRIPTS_DIR}/*.onnx")
	file(GLOB MODEL_DIRS RELATIVE "${SCRIPTS_DIR}/models" "models/*_onnx")
	# message(STATUS "Model dirs: ${MODEL_DIRS}")
	set(MODELS_BIN_DST "${CMAKE_BINARY_DIR}/models")

	if (MODEL_FILES AND NOT MODEL_FILES STREQUAL "")
		add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory ${MODELS_BIN_DST}

			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			    ${MODEL_FILES}
				${MODELS_BIN_DST}

			VERBATIM
		)
    endif()

	if (MODEL_DIRS AND NOT MODEL_DIRS STREQUAL "")
		foreach(dir ${MODEL_DIRS})
			add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
				    "${SCRIPTS_DIR}/${dir}"
					"${MODELS_BIN_DST}/${dir}"
				DEPENDS "${SCRIPTS_DIR}/${dir}"
				COMMENT "Copying ${dir} to ${MODELS_BIN_DST}/${dir}"
			)
	    endforeach()
	endif()
endfunction() # apss_copy_runtime_files

function(apss_copy_config_file src dst)
	message(STATUS "${src} ${dst}")

	add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${dst}

		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		    ${src}
			${dst}

		VERBATIM
	)
endfunction()

# Scans all the header files of persistent class targets and generates the persistent code.
# A more advanced version of the first. Only operates if the results don't exist or any changes were made to the sources.
function(apss_generate_odb_models db_type dst_dir)
	# Justification: The add_custom_command way of doing this somehow didn't work for me and I couldn't waste more time.

	message(STATUS "\n\n\t --- Converting persistent classes --- ")
	execute_process(COMMAND ${odb_EXECUTABLE} --version)

	message(STATUS "Creating directory: ${dst_dir}")
	file(MAKE_DIRECTORY ${dst_dir})

	# Locate the Qt headers for odb.exe
	execute_process(
		COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_HEADERS
		OUTPUT_VARIABLE QT_HEADERS
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

    set(_odb_generated_srcs "")
	set(_odb_generated_headers "")

	# Run odb.exe for each header conditionally
	foreach(src_file IN LISTS ARGN) # ARGN contains all extra arguments (.h files)
		get_filename_component(base_name "${src_file}" NAME_WE) # e.g., "event" from "event.h"

		# Create expected output file paths with their names
		set(gen_cxx_file "${dst_dir}/${base_name}-odb.cxx")
		set(gen_hxx_file "${dst_dir}/${base_name}-odb.hxx")
		set(gen_ixx_file "${dst_dir}/${base_name}-odb.ixx")

		# Collect generated file paths for parent scope regardless of whether they are re-generated
		list(APPEND _odb_generated_srcs "${gen_cxx_file}")
		list(APPEND _odb_generated_headers "${gen_hxx_file}" "${gen_ixx_file}")

		set(rebuild_this_file_required FALSE)

		# Create a unique cache variable name for this specific source file's last generation timestamp
		set(CACHE_VAR_NAME "ODB_${base_name}h_LAST_GEN_TIMESTAMP")

		# Load the previous timestamp from cache for this file
		# If it's not defined, it means this is the first time or cache was cleared.
		get_property(PREVIOUS_GEN_TIMESTAMP CACHE ${CACHE_VAR_NAME} PROPERTY VALUE)

		if (NOT EXISTS "${gen_cxx_file}" OR NOT EXISTS "${gen_hxx_file}" OR NOT EXISTS "${gen_ixx_file}")
			# Output files (.hxx, .ixx & .cxx) doesn't exist
			message(STATUS "\t-> Output files for '${src_file}' are missing. Rebuilding.")
			set(rebuild_this_file_required TRUE)
		else()
			# Output files exist, check modification times
			file(TIMESTAMP "${src_file}" SRC_TIMESTAMP UTC)
			# message(NOTICE "${src_file}: ${SRC_TIMESTAMP}, ${CACHE_VAR_NAME}: ${PREVIOUS_GEN_TIMESTAMP}")

			if (NOT SRC_TIMESTAMP)
				message(WARNING "\t-> Could not get timestamp for source file: ${src_file}. Forcing rebuild.")
				set(rebuild_this_file_required TRUE)
			else()
				# Compare source timestamp with the cached timestamp for this specific file
				if (NOT PREVIOUS_GEN_TIMESTAMP OR SRC_TIMESTAMP STRGREATER PREVIOUS_GEN_TIMESTAMP)
					message(STATUS "\t-> Source '${src_file}' was modified since last generation. Rebuilding.")
					set(rebuild_this_file_required TRUE)
				else()
					# Fallback check: compare source timestamp with actual generated file timestamps.
					# This covers cases where cache variable might be out of sync (e.g., manual deletion or older CMake versions)
					file(TIMESTAMP "${gen_cxx_file}" CXX_TIMESTAMP UTC)
					file(TIMESTAMP "${gen_hxx_file}" HXX_TIMESTAMP UTC)
					file(TIMESTAMP "${gen_ixx_file}" IXX_TIMESTAMP UTC)

					set(OLDEST_GEN_TIMESTAMP "${CXX_TIMESTAMP}")
					if (IXX_TIMESTAMP AND IXX_TIMESTAMP VERSION_LESS OLDEST_GEN_TIMESTAMP)
						set(OLDEST_GEN_TIMESTAMP "${IXX_TIMESTAMP}")
					endif()
					if (HXX_TIMESTAMP AND HXX_TIMESTAMP VERSION_LESS OLDEST_GEN_TIMESTAMP)
						set(OLDEST_GEN_TIMESTAMP "${HXX_TIMESTAMP}")
					endif()

					if (SRC_TIMESTAMP STRGREATER OLDEST_GEN_TIMESTAMP)
						message(STATUS "\t-> Source '${src_file}' is newer than existing generated files. Rebuilding.")
						set(rebuild_this_file_required TRUE)
					else()
						message(STATUS "\t-> Files for '${src_file}' are up-to-date. Skipping.")
					endif()
				endif()
			endif()
		endif()

		# Execute ODB compiler only if rebuild is required
		if (rebuild_this_file_required)
			# message(STATUS "\t-- Converting: ${src_file}")
			execute_process(
				COMMAND
				    ${odb_EXECUTABLE} -d ${db_type} -q -s -o ${dst_dir} --profile qt --std c++17 --schema-format separate
					-I${libodb_ROOT}/include "${src_file}" -I${QT_HEADERS}
					RESULT_VARIABLE ODB_CONV_RESULT
			)

		    if (ODB_CONV_RESULT EQUAL 0)
				# message(STATUS "\tConverted file ${src_file}")

				set(${CACHE_VAR_NAME} ${SRC_TIMESTAMP} CACHE STRING "Timestamp of last ODB generation for ${src_file}" FORCE)
				# message(STATUS "\t\tUpdated cache for '${src_file}' to ${SRC_TIMESTAMP}")
			else()
				message(FATAL_ERROR "\tFailed to convert file: ${src_file} (Error code: ${ODB_CONV_RESULT}). Check ODB output above for details.")
			endif()
		endif()
	endforeach()

	message(STATUS "--- Conversion finished ---\n\n")

	# Pass the generated source and header files back to the parent scope
	set(ODB_GENERATED_SRCS "${_odb_generated_srcs}" PARENT_SCOPE)
	set(ODB_GENERATED_HEADERS "${_odb_generated_headers}" PARENT_SCOPE)

endfunction() # apss_generate_odb_models


set(THIS_FILE ${CMAKE_CURRENT_LIST_FILE})
function(apss_patch_odb_pc_files libodb_ROOT)
	# Changes any \\ in paths to /. Because for some reason, pkgconf is not able to see \\ or something else
	# is going on and we get ba...., when reading b\\a\\..\\..

	message(NOTICE "Re-writing libodb's .pc files for windows specific paths (see ${THIS_FILE}: ${CMAKE_CURRENT_LIST_LINE})")
	file(GLOB	ODB_PC_FILES		LIST_DIRECTORIES FALSE "${libodb_ROOT}/lib/pkgconfig/*.pc")
	set(temp_pc_files
		${ODB_PC_FILES}
	)

    foreach(pc_file ${temp_pc_files})
		file(READ ${pc_file} PC_FILE_DATA)

		# message(STATUS "PC file at ${pc_file} had: ${PC_FILE_DATA}")
		string(REPLACE "\\\\" "/" PC_FILE_DATA ${PC_FILE_DATA})	# \\\\ results in \\.
		# message(STATUS "PC file at ${pc_file} now has: ${PC_FILE_DATA}")

		file(WRITE ${pc_file} ${PC_FILE_DATA})
	endforeach()
	message(NOTICE "Re-writing .pc files finished")

endfunction()
