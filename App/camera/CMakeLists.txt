# APSS Camera

add_library(APSSCamera STATIC
	cameracapture.cpp
	camerametrics.cpp
	cameraprocessor.cpp
	camerawatchdog.cpp
)
add_library(APSS::Camera ALIAS APSSCamera)

target_link_libraries(APSSCamera PRIVATE
	APSS::Config
	APSS::Predictors
)
