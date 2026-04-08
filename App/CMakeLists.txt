add_library(APSSLib STATIC
    camera/cameracapture.cpp
    camera/camerametrics.cpp
    camera/cameraprocessor.cpp
    camera/camerawatchdog.cpp

    db/event-odb.cxx
	db/frameprediction-odb.cxx
	db/recording-odb.cxx

    detectors/image.cpp
	detectors/lpdetectorsession.cpp
	detectors/objectdetector.cpp
	detectors/objectdetectorsession.cpp
	detectors/onnxinference.cpp
	detectors/paddlecls.cpp
	detectors/paddledet.cpp
	detectors/paddleocr.cpp
	detectors/paddlerec.cpp
	detectors/poseestimator.cpp
	detectors/predictor.cpp

	# Licensed
	detectors/licensed/clipper.cpp
	detectors/licensed/postprocess_op.cpp
	detectors/licensed/preprocess_op.cpp
	detectors/licensed/utility.cpp

    engine/apssengine.cpp

    models/camerametricsmodel.cpp
	models/eventsmodel.cpp

    output/packetringbuffer.h
    output/packetringbuffer.cpp
	output/perobjectremuxer.cpp
	output/recordingsmanager.cpp
	output/trackedobjectprocessor.cpp

    track/tracker.cpp

    utils/frame.cpp
	utils/framemanager.cpp
)

target_compile_definitions(APSSLib PRIVATE
  $<$<BOOL:${APSS_SUPPORT_CUDA_EP}>:APSS_SUPPORT_CUDA_EP>
  $<$<BOOL:${APSS_SUPPORT_OPENVINO_EP}>:APSS_SUPPORT_OPENVINO_EP>
)

target_include_directories(APSSLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${ODB_INCLUDE_DIRS}
)

target_link_libraries(APSSLib PUBLIC
    Qt6::Core
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::Sql

    cppzmq
    TBB::tbb
    Eigen3::Eigen
    bytetrackeigen
    onnxruntime::onnxruntime
    reflectcpp::reflectcpp
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
    ${ODB_LIBRARIES}
)

# APSSLib itself is a STATIC library. For an obvious reason, the compiler optimizes out the schema code
# of STATIC library builds, resulting in errors during runtime. So, we have to compile the schema code explicitly
# into the executable. But since the application executable is defined before this file (is included),
# include_directories won't reveal the includes to this main target and we have to do it explicitly.
target_sources(${CMAKE_PROJECT_NAME} PUBLIC
    db/event-schema.cxx
    db/frameprediction-schema.cxx
    db/recording-schema.cxx

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

    APSSLib

    TBB::tbb
    Eigen3::Eigen
    ByteTrackEigen::ByteTrack
    onnxruntime::onnxruntime
    reflectcpp::reflectcpp
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
    ${ODB_LIBRARIES}
)
