# APSS Events

add_library(APSSEvents INTERFACE)
add_library(APSS::Events ALIAS APSSEvents)

target_include_directories(APSSEvents INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(APSSEvents INTERFACE
	Qt6::Core
	cppzmq-static
)
