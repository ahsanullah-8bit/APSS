
# APSS Tests

add_executable(${CMAKE_PROJECT_NAME}Test
	tst_config.cpp
	tst_db_event.cpp
	tst_db_exports.cpp
	tst_db_recording.cpp
	tst_db_region.cpp
	tst_db_timeline.cpp
	tst_predictors.cpp
	tst_predictors_paddleocr.cpp
	# tst_predictors.cpp
	# tst_timeboundedqueue.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME}Test PRIVATE
	${CMAKE_SOURCE_DIR}/App	# Config and Database already has this as public, but anyway...
	${ODB_INCLUDE_DIRS}
)
target_link_directories(${CMAKE_PROJECT_NAME}Test PRIVATE ${ODB_LIBRARY_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME}Test PRIVATE
	Qt6::Core
	GTest::gtest
	GTest::gtest_main
	reflectcpp::reflectcpp
	TBB::tbb
	${ODB_LIBRARIES}
	onnxruntime::onnxruntime

	APSS::Config
	APSS::Database
	APSS::Predictors
)

include(GoogleTest)
gtest_add_tests(TARGET ${CMAKE_PROJECT_NAME}Test)

# Copy the assets
file(GLOB TEST_ASSETS_FILES LIST_DIRECTORIES FALSE "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
set(TEST_ASSETS_BIN_DST "${CMAKE_BINARY_DIR}/test/assets")
if (TEST_ASSETS_FILES AND NOT TEST_ASSETS_FILES STREQUAL "")
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}Test POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_ASSETS_BIN_DST}

		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		    ${TEST_ASSETS_FILES}
			${TEST_ASSETS_BIN_DST}

		VERBATIM
	)
endif()
