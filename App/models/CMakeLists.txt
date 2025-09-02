add_library(APSSModels STATIC
	camerametricsmodel.cpp
	eventsmodel.cpp
)

add_library(APSS::Models ALIAS APSSModels)
