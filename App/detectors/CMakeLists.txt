# Configuration specific to detectors/predictors

# target_sources(${CMAKE_PROJECT_NAME} PUBLIC
# 	objectdetector.cpp
# 	objectdetectorsession.cpp
# 	poseestimator.cpp
# 	predictor.cpp
# 	image.cpp
# )


add_library(APSSPredictors STATIC
	image.cpp
	lpdetectorsession.cpp
	objectdetector.cpp
	objectdetectorsession.cpp
	onnxinference.cpp
	paddlecls.cpp
	paddledet.cpp
	paddleocr.cpp
	paddlerec.cpp
	poseestimator.cpp
	predictor.cpp

	# Licensed
	licensed/clipper.cpp
	licensed/postprocess_op.cpp
	licensed/preprocess_op.cpp
	licensed/utility.cpp
)
add_library(APSS::Predictors ALIAS APSSPredictors)

target_link_libraries(APSSPredictors PRIVATE
	APSS::Config
	APSS::Utils
)

target_compile_definitions(APSSPredictors PRIVATE
  $<$<BOOL:${APSS_SUPPORT_CUDA_EP}>:APSS_SUPPORT_CUDA_EP>
  $<$<BOOL:${APSS_SUPPORT_OPENVINO_EP}>:APSS_SUPPORT_OPENVINO_EP>
)
