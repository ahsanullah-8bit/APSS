set(APSS_ORM_HEADERS
   models/event.h
   models/export.h
   models/orm_export.h
   models/recordings.h
   models/regions.h
   models/timeline.h
)
set(APSS_ORM_SRCS
   models/event.cpp
   models/export.cpp
   models/recordings.cpp
   models/regions.cpp
   models/timeline.cpp
)

# if(NOT TARGET QxOrm)
#    add_subdirectory("${CMAKE_SOURCE_DIR}/Dependencies/QxOrm" .QxOrm)
# endif() # NOT TARGET QxOrm

####################################################
# Macro To Source Group By Folder For MSVC / XCode #
####################################################

macro(qx_auto_source_group QX_ALL_FILES)
   foreach(QX_FILE ${QX_ALL_FILES})
      get_filename_component(QX_PARENT_DIR "${QX_FILE}" DIRECTORY)
      string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" QX_GROUP "${QX_PARENT_DIR}")
      string(REPLACE "./" "" QX_GROUP "${QX_GROUP}")
      string(REPLACE "/" "\\" QX_GROUP "${QX_GROUP}")
      source_group("${QX_GROUP}" FILES "${QX_FILE}")
   endforeach()
endmacro() # qx_auto_source_group

# This code was copied from QxOrm/QxOrm.cmake due to the weird structure of QxOrm's whole CMake
# configuration and the way this project is structured. May cause redefinition errors
####################################################


target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE -D_BUILDING_APSS_ORM)

if(COMMAND target_precompile_headers)
   target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE
	    "${CMAKE_CURRENT_SOURCE_DIR}/models/orm_precompiled.h"
   )
endif() # (COMMAND target_precompile_headers)

target_sources(${CMAKE_PROJECT_NAME} PUBLIC ${APSS_ORM_HEADERS} ${APSS_ORM_SRCS})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/Dependencies/QxOrm/include")
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/Dependencies/QxOrm/lib")
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${QX_LIBRARIES} QxOrm)

qx_auto_source_group("${APSS_ORM_SRCS};${APSS_ORM_HEADERS}")
