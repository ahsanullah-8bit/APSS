include(CMakePackageConfigHelpers)

# set(VCPKG_POLICY_SKIP_COPYRIGHT_CHECK enabled)

# This archive contains just the libodb-qt.
# SEE SECTION 1 (below) FOR MORE DETAILS
if (VCPKG_TARGET_IS_WINDOWS)
    set(ODB_URL "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/libodb-qt-2.5.0-x64-windows-prebuilt.tar")
endif()

vcpkg_download_distfile(ARCHIVE
    URLS ${ODB_URL}
    FILENAME "libodb-qt-2.5.0.tar"
    SHA512 be43fafefa0cfd5b7ce349c098c15e9c675f4b69f9ef1a5e9d4970c617c068b7976c5c5d4604564c652b747b4008f3e243a109e99b51df4760c7276331935440
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
# https://codesynthesis.com/download/odb/2.5.0/windows/windows10/x86_64/libodb-qt-2.5.0-x86_64-windows10-msvc17.10-debug.zip

# NOTE: These binaries have got a problem with there debug naming or something. I couldn't fix it at the time but I'll, soon.
#       There is no libodb-qt 2.5.0 in vcpkg as of now.