# WARNING: You should mention absolute paths, because the function definition takes
# it from its own directory's perspective
set(ODB_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/event.h
	${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/exports.h
	${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/recording.h
	${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/region.h
	${CMAKE_CURRENT_SOURCE_DIR}/odbmodels/timeline.h
)
apss_generate_odb_models3("sqlite"
	${CMAKE_CURRENT_SOURCE_DIR}/sqlite
	${ODB_HEADERS})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
	odbmodels	# For the generated models includes to work properly
	sqlite
	${CMAKE_CURRENT_SOURCE_DIR}
	${ODB_INCLUDE_DIRS}
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
	sqlite/event-odb.cxx
	sqlite/exports-odb.cxx
	sqlite/recording-odb.cxx
	sqlite/region-odb.cxx
	sqlite/timeline-odb.cxx
)

target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${ODB_LIBRARY_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
	${ODB_LIBRARIES}
)

# message(STATUS "CFlags ${ODB_CFLAGS}")
# message(STATUS "LDFlags ${ODB_LDFLAGS}")
# message(STATUS "Link directories ${ODB_LIBRARY_DIRS}")
# message(STATUS "Link libs ${ODB_LIBRARIES}")
# message(STATUS "Include Dirs ${ODB_INCLUDE_DIRS}")
