# APSS Utils

add_library(APSSUtils STATIC
	frame.cpp
	framemanager.cpp
)
add_library(APSS::Utils ALIAS APSSUtils)

# target_precompile_headers(APSSUtils INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pch.h>")
target_include_directories(APSSUtils PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(APSSUtils PUBLIC
	Qt6::Core
	Qt6::Multimedia
	${OpenCV_LIBS}

	APSS::Config
	APSS::Predictors
)
