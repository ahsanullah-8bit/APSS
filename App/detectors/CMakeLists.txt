# Configuration specific to detectors/predictors

target_sources(${CMAKE_PROJECT_NAME} PUBLIC
	objectdetector.cpp
	objectdetectorsession.cpp
	poseestimator.cpp
	predictor.cpp
)


add_library(APSSPredictors STATIC
	onnxinference.cpp
	paddlecls.cpp
	paddledet.cpp
	paddleocr.cpp
	paddlerec.cpp

	# Licensed
	licensed/clipper.cpp
	licensed/postprocess_op.cpp
	licensed/preprocess_op.cpp
	licensed/utility.cpp
)
add_library(APSS::Predictors ALIAS APSSPredictors)

target_include_directories(APSSPredictors PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..			# Not a good choice, but anyway...

	${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(APSSPredictors PRIVATE
	Qt6::Core
	onnxruntime::onnxruntime
	${OpenCV_LIBS}

	APSS::Config
)

target_compile_definitions(APSSPredictors PRIVATE
  $<$<BOOL:${APSS_SUPPORT_CUDA_EP}>:APSS_SUPPORT_CUDA_EP>
  $<$<BOOL:${APSS_SUPPORT_OPENVINO_EP}>:APSS_SUPPORT_OPENVINO_EP>
)

message(STATUS "${OpenCV_LIBS}")
