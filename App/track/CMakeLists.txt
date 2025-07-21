# APSS Track

add_library(APSSTrack STATIC
	tracker.cpp
)
add_library(APSS::Track ALIAS APSSTrack)

target_include_directories(APSSTrack PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(APSSTrack PUBLIC
	Qt6::Core

	Eigen3::Eigen
	ByteTrackEigen::ByteTrack
	${OpenCV_LIBS}

	APSS::Utils
	APSS::Config
)
