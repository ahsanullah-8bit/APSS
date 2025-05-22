
set(${CMAKE_INCLUDE_CURRENT_DIR} ON)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR} # For the export.h to be found.
    camera
    detectors
    engine
    utils

    # ${OnnxRuntime_INCLUDE_DIR}
    "${CMAKE_SOURCE_DIR}/Dependencies/ByteTrackEigen/include"
)

target_sources(${CMAKE_PROJECT_NAME} PUBLIC
    "main.cpp" 
    camera/filestream.cpp
    camera/ffmpegcamera.cpp
    camera/ffmpegfile.cpp
    detectors/yoloinference.cpp
    detectors/yolodetection.cpp
    detectors/yolosegmentation.cpp
    detectors/yolopose.cpp
    engine/apssengine.cpp
    engine/objectdetectorsession.cpp
    engine/lprsession.cpp
    engine/boundedinformer.h
)

target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Multimedia

    TBB::tbb
    ${FFMPEG_LIBRARIES}
    ${OpenCV_LIBS}
    bytetrackeigen  # Can't use alias, because it's a sub-project
)

add_subdirectory(db)
