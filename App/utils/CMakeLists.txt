# APSS Utils

add_library(APSSUtils STATIC
	frame.cpp
	framemanager.cpp
)
add_library(APSS::Utils ALIAS APSSUtils)

target_link_libraries(APSSUtils PUBLIC
	APSS::Config
	APSS::Predictors
)
