# APSS Config

add_library(APSSConfig INTERFACE)
add_library(APSS::Config ALIAS APSSConfig)

target_precompile_headers(APSSConfig INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pch.h>")
target_include_directories(APSSConfig INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(APSSConfig INTERFACE reflectcpp::reflectcpp)
