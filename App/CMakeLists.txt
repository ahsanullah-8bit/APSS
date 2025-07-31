# Every sub-folder has its own configuration. Because of weird modifications QDS do to this file sometimes...

add_subdirectory(camera)
add_subdirectory(config)
add_subdirectory(db)
add_subdirectory(detectors)
add_subdirectory(events)
add_subdirectory(track)
add_subdirectory(utils)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ODB_INCLUDE_DIRS}
)

target_sources(${CMAKE_PROJECT_NAME} PUBLIC
    db/sqlite/event-schema.cxx
    db/sqlite/frameprediction-schema.cxx
    db/sqlite/recording-schema.cxx

    engine/apssengine.cpp
    engine/trackedobjectprocessor.cpp
    models/camerametricsmodel.cpp
    models/eventsmodel.cpp
    output/recordingsmanager.cpp
    main.cpp
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
    APSS::Events
    APSS::Predictors
    APSS::Track
    APSS::Utils

    TBB::tbb
    Eigen3::Eigen
    ByteTrackEigen::ByteTrack
    onnxruntime::onnxruntime
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
    ${ODB_LIBRARIES}
)
