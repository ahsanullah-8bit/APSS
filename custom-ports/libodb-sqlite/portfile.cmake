include(CMakePackageConfigHelpers)

# set(VCPKG_POLICY_SKIP_COPYRIGHT_CHECK enabled)

# This archive contains just the libodb-sqlite.
# SEE SECTION 1 (below) FOR MORE DETAILS
if (VCPKG_TARGET_IS_WINDOWS)
    set(ODB_URL "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/libodb-sqlite-2.5.0-x64-windows-prebuilt.tar")
endif()

vcpkg_download_distfile(ARCHIVE
    URLS ${ODB_URL}
    FILENAME "libodb-sqlite-2.5.0.tar"
    SHA512 8245dbcaaad947f3747329c055148eae08ea17ee797ac168803ebbebf514ef4ef6c0292e58a85be363b4e313794155f55b57b5b55cb68a4ed240897d71e4d247
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
    file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright" "Prebuilt binaries from ${ODB_URL}")
endif()

# Section 1: Prebuilt binaries for Windows (by the maintainers)
# https://codesynthesis.com/download/odb/2.5.0/windows/windows10/x86_64/libodb-sqlite-2.5.0-x86_64-windows10-msvc17.10-debug.zip

# NOTE: These binaries have got a problem with there debug naming or something. I couldn't fix it at the time but I'll, soon.
#       There is no libodb-sqlite 2.5.0 in vcpkg as of now.