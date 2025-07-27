# WARNING: You should mention absolute paths, because the function definition takes
# it from its own directory's perspective
set(ODB_HEADERS
	# ${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/camera.h
	${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/event.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/exports.h
	${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/frameprediction.h
	${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/recording.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/region.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/timeline.h
)
apss_generate_odb_models4("sqlite"
	${CMAKE_CURRENT_SOURCE_DIR}/sqlite
	${ODB_HEADERS})

add_library(APSSDatabase STATIC
	sqlite/event-odb.cxx
	# sqlite/exports-odb.cxx
	sqlite/frameprediction-odb.cxx
	sqlite/recording-odb.cxx
	# sqlite/region-odb.cxx
	# sqlite/timeline-odb.cxx
)
add_library(APSS::Database ALIAS APSSDatabase)

target_precompile_headers(APSSDatabase PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/pch.h>")
target_include_directories(APSSDatabase PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	odbmodels
	sqlite
)
target_include_directories(APSSDatabase PRIVATE ${ODB_INCLUDE_DIRS})
target_link_directories(APSSDatabase PUBLIC ${ODB_LIBRARY_DIRS})
target_link_libraries(APSSDatabase PUBLIC
	Qt6::Core
	${ODB_LIBRARIES}
)

# Debug
# message(STATUS "CFlags ${ODB_CFLAGS}")
# message(STATUS "LDFlags ${ODB_LDFLAGS}")
# message(STATUS "Link directories ${ODB_LIBRARY_DIRS}")
# message(STATUS "Link libs ${ODB_LIBRARIES}")
# message(STATUS "Include Dirs ${ODB_INCLUDE_DIRS}")
