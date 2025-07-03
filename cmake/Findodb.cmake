# Defines custom find_package package and targets

######################################
##		libodb 2.5.0
######################################

set(ODB_LINK "https://www.codesynthesis.com/download/odb/2.5.0/windows/windows10/x86_64")
set(ODB_EXE_ARCHIVE "${ODB_LINK}/odb-2.5.0-x86_64-windows10.zip")
set(ODB_ARCHIVE "${ODB_LINK}/libodb-2.5.0-x86_64-windows10-msvc17.10.zip")
set(ODB_QT_ARCHIVE "${ODB_LINK}/libodb-qt-2.5.0-x86_64-windows10-msvc17.10.zip")
set(ODB_SQLITE_ARCHIVE "${ODB_LINK}/libodb-sqlite-2.5.0-x86_64-windows10-msvc17.10.zip")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(ODB_ARCHIVE "${ODB_LINK}/libodb-2.5.0-x86_64-windows10-msvc17.10-debug.zip")
	set(ODB_QT_ARCHIVE "${ODB_LINK}/libodb-qt-2.5.0-x86_64-windows10-msvc17.10-debug.zip")
	set(ODB_SQLITE_ARCHIVE "${ODB_LINK}/libodb-sqlite-2.5.0-x86_64-windows10-msvc17.10-debug.zip")
	set(ODB_DSYMB "D")
endif()

# odb_exe
FetchContent_Declare(odbexe
	URL ${ODB_EXE_ARCHIVE}
)
# libodb
FetchContent_Declare(libodb
	URL ${ODB_ARCHIVE}
)
# libodb_qt
FetchContent_Declare(libodbqt
	URL ${ODB_QT_ARCHIVE}
)
# libodb_sqlite
FetchContent_Declare(libodbsqlite
	URL ${ODB_SQLITE_ARCHIVE}
)
FetchContent_MakeAvailable(odbexe libodb libodbqt libodbsqlite)

# Expected by odb.exe
set(odb_EXECUTABLE "${odbexe_SOURCE_DIR}/bin/odb.exe" CACHE STRING "Path to the odb.exe")
set(libodb_ROOT ${libodb_SOURCE_DIR} CACHE STRING "Path to the libodb directory")
set(libodb_qt_ROOT ${libodbqt_SOURCE_DIR} CACHE STRING "Path to the libodb-qt directory")
set(libodb_sqlite_ROOT ${libodbsqlite_SOURCE_DIR} CACHE STRING "Path to the libodb-sqlite directory")

file(GLOB	ODB_FILES			LIST_DIRECTORIES FALSE "${libodb_ROOT}/bin/*.dll")
file(GLOB	ODB_QT_FILES		LIST_DIRECTORIES FALSE "${libodb_qt_ROOT}/bin/*.dll")
file(GLOB	ODB_SQLITE_FILES	LIST_DIRECTORIES FALSE "${libodb_sqlite_ROOT}/bin/*.dll")
set(APSS_POSTBUILD_RUNTIME_FILES  ${APSS_POSTBUILD_RUNTIME_FILES} ${ODB_FILES} ${ODB_QT_FILES} ${ODB_SQLITE_FILES})

# Find the damn packages
# We find the main/core libodb, explicitly.
list(APPEND CMAKE_PREFIX_PATH ${libodb_ROOT} ${libodb_qt_ROOT} ${libodb_sqlite_ROOT})

find_package(PkgConfig QUIET)
pkg_check_modules(PC_libodb QUIET libodb libodb-qt)

message(STATUS "CFlags ${PC_libodb_CFLAGS}")
message(STATUS "LDFlags ${PC_libodb_LDFLAGS}")

find_path(libodb_INCLUDE_DIR
	NAMES odb/version.hxx
	HINTS
	    ${libodb_ROOT}
		${PC_libodb_INCLUDE_DIRS}
)
find_library(libodb_LIBRARY
	NAMES "odb" "odb-2.5" "odbD" "odbD-2.5"
	HINTS
	    ${libodb_ROOT}
		${PC_libodb_LIBRARY_DIRS}
)

find_file(libodb_RUNTIME
	NAMES "odb.dll" "odb-2.5.dll" "odbD.dll" "odbD-2.5.dll"
	HINTS
	    ${libodb_ROOT}/bin
	NO_CACHE
)

set(odb_INCLUDE_DIRS ${libodb_INCLUDE_DIR} CACHE STRING "Paths to libodb includes" FORCE)
set(odb_LIBRARIES ${libodb_LIBRARY} CACHE STRING "Libs inside libodb" FORCE)

mark_as_advanced(libodb_INCLUDE_DIR libodb_LIBRARY)

if (odb_INCLUDE_DIRS AND odb_LIBRARIES)
	set(odb_FOUND TRUE)
	# Targets
	if (NOT TARGET odb::libodb)
		add_library(odb::libodb SHARED IMPORTED)
		set_property(TARGET odb::libodb
			PROPERTY IMPORTED_CONFIGURATIONS RELEASE DEBUG)
		set_property(TARGET odb::libodb
			PROPERTY IMPORTED_IMPLIB_DEBUG ${libodb_LIBRARY}
			PROPERTY IMPORTED_IMPLIB_RELEASE ${libodb_LIBRARY}
			PROPERTY IMPORTED_LOCATION_DEBUG ${libodb_RUNTIME}
			PROPERTY IMPORTED_LOCATION_RELEASE ${libodb_RUNTIME}
		)
	    target_include_directories(odb::libodb INTERFACE ${libodb_INCLUDE_DIR})
	endif()
endif()

foreach(component ${odb_FIND_COMPONENTS})
	apss_find_odb_api(${component} ${libodb_${component}_ROOT})
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(odb
	REQUIRED_VARS odb_FOUND
	HANDLE_COMPONENTS)
