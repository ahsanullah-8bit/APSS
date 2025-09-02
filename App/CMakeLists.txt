# Most of the dependencies are common to the sub targets. So, to avoid the pain, why not just include_directories as the whole application was
# including headers and linking libraries in a single place anyway.
# Note: This includes tests as they're part of the same project.
include_directories(AFTER
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${ODB_INCLUDE_DIRS}
)

link_directories(AFTER
    ${FFMPEG_LIBRARY_DIRS}
    ${ODB_LIBRARY_DIRS}
)

link_libraries(
    Qt6::Core
    Qt6::Multimedia

    TBB::tbb
    Eigen3::Eigen
    ByteTrackEigen::ByteTrack
    onnxruntime::onnxruntime
    reflectcpp::reflectcpp
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
    ${ODB_LIBRARIES}
)

# Every sub-folder has its own configuration. Because of some weird modifications QDS do to this file sometimes...
add_subdirectory(camera)
add_subdirectory(config)
add_subdirectory(db)
add_subdirectory(detectors)
add_subdirectory(events)
add_subdirectory(models)
add_subdirectory(output)
add_subdirectory(track)
add_subdirectory(utils)

# link_libraries(
#     APSS::Camera
#     APSS::Config
#     APSS::Database
#     APSS::Predictors
#     APSS::Events
#     APSS::Models
#     APSS::Output
#     APSS::Track
#     APSS::Utils
# )

# APSS::Database itself is a STATIC library. For an obvious reason, the compiler optimizes out the schema code
# of STATIC library builds, resulting in errors during runtime. So, we have to compile the schema code explicitly
# into the executable. But since the application executable is defined before this file (is included),
# include_directories won't reveal the includes to this main target and we have to do it explicitly.
target_sources(${CMAKE_PROJECT_NAME} PUBLIC
    db/sqlite/event-schema.cxx
    db/sqlite/frameprediction-schema.cxx
    db/sqlite/recording-schema.cxx

    engine/apssengine.cpp
    main.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${FFMPEG_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${ODB_INCLUDE_DIRS}
)
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${FFMPEG_LIBRARY_DIRS}
    ${ODB_LIBRARY_DIRS}
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::Sql

    APSS::Camera
    APSS::Config
    APSS::Database
    APSS::Predictors
    APSS::Events
    APSS::Models
    APSS::Output
    APSS::Track
    APSS::Utils

    TBB::tbb
    Eigen3::Eigen
    ByteTrackEigen::ByteTrack
    onnxruntime::onnxruntime
    reflectcpp::reflectcpp
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
    ${ODB_LIBRARIES}
)
