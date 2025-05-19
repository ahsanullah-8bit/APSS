# This file handles downloading and extracting of prebuilt binaries

include (FetchContent)
set(FetchContent_QUIET OFF)

# Only works for windows, as of now.
if (WIN32)
	FetchContent_Declare(ffmpeg
		SOURCE_DIR "${CMAKE_SOURCE_DIR}/_deps/ffmpeg_7.1"
		UPDATES_DISCONNECTED ON
		GIT_PROGRESS ON
		URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n7.1-latest-win64-lgpl-shared-7.1.zip"
	)
    FetchContent_MakeAvailable(ffmpeg)
	FetchContent_GetProperties(ffmpeg)

	set(PKG_CONFIG_PATH "${ffmpeg_SOURCE_DIR}/lib/pkgconfig" CACHE PATH "Path to FFmpeg pkg-config files")
	find_package(PkgConfig REQUIRED)

	pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libswscale libavutil)
endif()
