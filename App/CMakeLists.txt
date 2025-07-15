# Every sub-folder has its own configuration. Because of weird modifications QDS do to this file sometimes...

add_subdirectory(camera)
add_subdirectory(config)
add_subdirectory(db)
add_subdirectory(detectors)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    # camera
    # config
    # detectors
    # engine
    # utils
)

target_sources(${CMAKE_PROJECT_NAME} PUBLIC
    engine/apssengine.cpp
    models/camerametricsmodel.cpp
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

    APSS::Config
    APSS::Database
    APSS::Predictors

    TBB::tbb
    Eigen3::Eigen
    ByteTrackEigen::ByteTrack
    onnxruntime::onnxruntime
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
    ${ODB_LIBRARIES}
)
