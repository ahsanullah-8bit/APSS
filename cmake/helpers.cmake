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
				URL "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/onnxruntime-win-x64-1.21.1-cpu-ov.zip"
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
				URL "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/openvino-2025.1.0-win-x64-auto-cpu-gpu-.zip"
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
		if (NOT odb_EXECUTABLE OR NOT odb_EXECUTABLE STREQUAL "")
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

		# Chnage the \\ in every .pc files to /, because pkgconf is reading \\..\\.. as .... from the .pc files..
		apss_rewrite_odb_pc_files(${libodb_ROOT})

		######################################

		# Debug
		# message(STATUS "onnxruntime_ROOT: ${onnxruntime_DIR}")
		# message(STATUS "OpenVINO_ROOT: ${OpenVINO_ROOT}")
		# message(STATUS "odb_EXECUTABLE: ${odb_EXECUTABLE}")
		# message(STATUS "libodb_ROOT: ${libodb_ROOT}")

		######################

		# NOTE: libx264-164.dll used to get copied to the binary dir as of ffmpeg v7.1 but now it doesn't
		#		https://github.com/microsoft/vcpkg/issues/43802
		file(GLOB_RECURSE	ORT_DLLS			LIST_DIRECTORIES FALSE "${onnxruntime_ROOT}/*.dll") # <dir>/lib/cmake/onnxruntime
		file(GLOB_RECURSE	OPENVINO_DLLS		LIST_DIRECTORIES FALSE "${OpenVINO_ROOT}/runtime/*.dll")
		file(GLOB			OPENVINO_JSON		LIST_DIRECTORIES FALSE "${OpenVINO_ROOT}/runtime/bin/intel64/Release/*.json")
		file(GLOB			x264_DLLS			LIST_DIRECTORIES FALSE "${VCPKG_INSTALLED_DIR}/x64-windows/tools/x264/bin/*.dll")
		file(GLOB			ODB_FILES			LIST_DIRECTORIES FALSE "${libodb_ROOT}/bin/*.dll")

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

endfunction()

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
	file(GLOB MODEL_FILES LIST_DIRECTORIES FALSE "${CMAKE_SOURCE_DIR}/scripts/*.onnx")
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
endfunction()

# Scans all the header files of persistent class targets and generates the persistent code
function(apss_generate_odb_models db_type dst_dir)
	message(STATUS "\n\t --- Converting persistent classes --- ")
	# Get the odb.exe version
	execute_process(COMMAND ${odb_EXECUTABLE} --version)
	message(STATUS "Creating directory: ${dst_dir}")
	file(MAKE_DIRECTORY ${dst_dir})

	# Locate the Qt headers for odb.exe
	execute_process(
	  COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_HEADERS
	  OUTPUT_VARIABLE QT_HEADERS
	  OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Run odb.exe for each header
	foreach(file IN LISTS ARGN)  # ARGN contains all extra arguments beyond the expected ones
		execute_process(
			COMMAND
			    ${odb_EXECUTABLE} -d ${db_type} -q -s -o ${dst_dir} --profile qt --std c++17
				-I${libodb_ROOT}/include ${file} -I${QT_HEADERS}
			RESULT_VARIABLE ODB_CONV_RESULT
		)

	    if (ODB_CONV_RESULT EQUAL 0)
			message(STATUS "Converted file ${file}")
		else()
			message(WARNING "Error: ${ODB_CONV_RESULT}")
		endif()
	endforeach()

	message(STATUS " --- Conversion finished --- ")
endfunction()


# Scans all the header files of persistent class targets and generates the persistent code
function(apss_generate_odb_models3 db_type dst_dir)
	message(STATUS "\n\t --- Converting persistent classes (Configure Time Check & Cache) --- ")

	# Get the odb.exe version - run once during configure time
	execute_process(COMMAND ${odb_EXECUTABLE} --version)

	# Ensure the destination directory exists - create once during configure time
	message(STATUS "Creating directory: ${dst_dir}")
	file(MAKE_DIRECTORY ${dst_dir})

	# Locate the Qt headers for odb.exe - run once during configure time
	execute_process(
		COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_HEADERS
		OUTPUT_VARIABLE QT_HEADERS
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

    set(_odb_generated_srcs "")
	set(_odb_generated_headers "")

	# --- Cache variable to track the last successful generation timestamp ---
	# This variable acts as a sentinel. If the source file is newer than this timestamp,
	# we know we need to re-run ODB.
	# We will update this timestamp *after* successful ODB generation.
	set(ODB_LAST_GEN_TIMESTAMP "" CACHE STRING "Timestamp of last ODB model generation.")

	set(overall_rebuild_needed FALSE) # Track if *any* ODB file needed regeneration

	# Run odb.exe for each header conditionally
	foreach(src_file IN LISTS ARGN) # ARGN contains all extra arguments (.h files)
		get_filename_component(base_name "${src_file}" NAME_WE) # e.g., "event" from "event.h"

		set(gen_cxx_file "${dst_dir}/${base_name}-odb.cxx")
		set(gen_hxx_file "${dst_dir}/${base_name}-odb.hxx")
		set(gen_ixx_file "${dst_dir}/${base_name}-odb.ixx")

		# Collect generated file paths for parent scope regardless of whether they are re-generated
		list(APPEND _odb_generated_srcs "${gen_cxx_file}")
		list(APPEND _odb_generated_headers "${gen_hxx_file}" "${gen_ixx_file}")

		set(rebuild_this_file_required FALSE)

		# 1. Check if output files exist
		if (NOT EXISTS "${gen_cxx_file}" OR NOT EXISTS "${gen_hxx_file}" OR NOT EXISTS "${gen_ixx_file}")
			message(STATUS "\t-> Output files for '${src_file}' are missing. Rebuilding.")
			set(rebuild_this_file_required TRUE)
		else()
			# 2. If output files exist, check modification times
			file(TIMESTAMP "${src_file}" SRC_TIMESTAMP UTC)
			if (NOT SRC_TIMESTAMP)
				message(WARNING "  -> Could not get timestamp for source file: ${src_file}. Forcing rebuild.")
				set(rebuild_this_file_required TRUE)
			else()
				# Compare source timestamp with the stored ODB_LAST_GEN_TIMESTAMP
				# This is the key change: we check against the global generation timestamp
				if (ODB_LAST_GEN_TIMESTAMP AND SRC_TIMESTAMP VERSION_GREATER ODB_LAST_GEN_TIMESTAMP)
					message(STATUS "  -> Source '${src_file}' was modified. Rebuilding.")
					set(rebuild_this_file_required TRUE)
				else()
					# Also check if actual generated files are missing or older, as a fallback
					# This handles scenarios where ODB_LAST_GEN_TIMESTAMP might be out of sync
					# due to manual file deletion, etc.
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

					if (SRC_TIMESTAMP VERSION_GREATER OLDEST_GEN_TIMESTAMP)
						 message(STATUS "  -> Source '${src_file}' is newer than existing generated files. Rebuilding.")
						 set(rebuild_this_file_required TRUE)
					else()
						message(STATUS "  -> Files for '${src_file}' are up-to-date. Skipping.")
					endif()
				endif()
			endif()
		endif()

		# Execute ODB compiler only if rebuild is required
		if (rebuild_this_file_required)
			message(STATUS "\t-- Converting: ${src_file}")
			execute_process(
				COMMAND
				    ${odb_EXECUTABLE} -d ${db_type} -q -s -o ${dst_dir} --profile qt --std c++17
					-I${libodb_ROOT}/include "${src_file}" -I${QT_HEADERS}
					RESULT_VARIABLE ODB_CONV_RESULT
			)

		    if (ODB_CONV_RESULT EQUAL 0)
				message(STATUS "\tConverted file ${src_file}")
				set(overall_rebuild_needed TRUE) # Mark that at least one file was rebuilt
			else()
				message(FATAL_ERROR "\tFailed to convert file: ${src_file} (Error code: ${ODB_CONV_RESULT}). Check ODB output above for details.")
			endif()
		endif()
	endforeach()

	# Update the cache timestamp if any file was rebuilt
	if (overall_rebuild_needed)
		file(TIMESTAMP "${CMAKE_SOURCE_DIR}/CMakeLists.txt" CURRENT_TIME_STAMP UTC) # Get a fresh timestamp
		set(ODB_LAST_GEN_TIMESTAMP "${CURRENT_TIME_STAMP}" CACHE STRING "Timestamp of last ODB model generation." FORCE)
		message(STATUS "\tUpdated ODB_LAST_GEN_TIMESTAMP to ${ODB_LAST_GEN_TIMESTAMP}")
		# Force CMake to reconfigure on next run if this happens (this is the key to breaking the cycle)
		# This will make CMake re-evaluate the entire build graph.
		configure_file("${CMAKE_SOURCE_DIR}/CMakeLists.txt" "${CMAKE_CURRENT_BINARY_DIR}/.force_reconfigure_odb" @ONLY)
	endif()


	message(STATUS " --- Conversion finished --- ")

	# Pass the generated source and header files back to the parent scope
	set(ODB_GENERATED_SRCS "${_odb_generated_srcs}" PARENT_SCOPE)
	set(ODB_GENERATED_HEADERS "${_odb_generated_headers}" PARENT_SCOPE)

endfunction()

# Changes any \\ in paths to /. Because for some reason, pkgconf is not able to see \\ or something else
# is going on we get a...., when reading a\\..\\..
function(apss_rewrite_odb_pc_files libodb_ROOT)

	message(NOTICE "Re-writing libodb's .pc files")
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
