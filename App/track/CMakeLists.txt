# APSS Track

add_library(APSSTrack STATIC
	tracker.cpp
)
add_library(APSS::Track ALIAS APSSTrack)

target_link_libraries(APSSTrack PUBLIC
	APSS::Utils
	APSS::Config
)
