include(CMakePackageConfigHelpers)

# set(VCPKG_POLICY_SKIP_COPYRIGHT_CHECK enabled)

# This archive contains the sqlite3, provided with libodb.
if (VCPKG_TARGET_IS_WINDOWS)
    set(SQLITE3_URL "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/sqlite3-3.45.3-x64-windows-prebuilt.tar")
endif()

vcpkg_download_distfile(ARCHIVE
    URLS ${SQLITE3_URL}
    FILENAME "sqlite3-3.45.3.tar"
    SHA512 15db97e062c5ad16fc717752db374dd5ffec3da374ac6f83daf685f09429fd40aad040cb5c892dd5e57a30cc8d0c55d1af396a08e74ae91582dc3144cc166bba
)

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
    NO_REMOVE_ONE_LEVEL
)

file(INSTALL "${SOURCE_PATH}/bin" DESTINATION "${CURRENT_PACKAGES_DIR}")
file(INSTALL "${SOURCE_PATH}/include" DESTINATION "${CURRENT_PACKAGES_DIR}")
file(INSTALL "${SOURCE_PATH}/lib" DESTINATION "${CURRENT_PACKAGES_DIR}")
file(INSTALL "${SOURCE_PATH}/share" DESTINATION "${CURRENT_PACKAGES_DIR}")
file(INSTALL "${SOURCE_PATH}/debug" DESTINATION "${CURRENT_PACKAGES_DIR}")

if(NOT EXISTS "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright")
    file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright" "Prebuilt binaries from ${SQLITE3_URL}")
endif()