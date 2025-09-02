add_library(APSSOutput STATIC

	recordingsmanager.cpp
	trackedobjectprocessor.cpp
)

add_library(APSS::Output ALIAS APSSOutput)

target_link_libraries(APSSOutput PRIVATE
	APSS::Database
	APSS::Predictors
	APSS::Events
	APSS::Utils
)
