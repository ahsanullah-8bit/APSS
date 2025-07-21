# APSS Camera

add_library(APSSCamera STATIC
	cameracapture.cpp
	camerametrics.cpp
	cameraprocessor.cpp
	camerawatchdog.cpp
)
add_library(APSS::Camera ALIAS APSSCamera)

target_include_directories(APSSCamera PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_directories(APSSCamera PUBLIC
	${FFMPEG_LIBRARY_DIRS}
)
target_link_libraries(APSSCamera PUBLIC
	Qt6::Core
	Qt6::Multimedia

	TBB::tbb
	onnxruntime::onnxruntime
	ByteTrackEigen::ByteTrack
	${FFMPEG_LIBRARIES}
	${OpenCV_LIBS}

	APSS::Config
	APSS::Predictors
)
