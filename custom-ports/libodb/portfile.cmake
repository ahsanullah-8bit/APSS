include(CMakePackageConfigHelpers)

# set(VCPKG_POLICY_SKIP_COPYRIGHT_CHECK enabled)

# This archive contains just the libodb.
# SEE SECTION 1 (below) FOR MORE DETAILS
if (VCPKG_TARGET_IS_WINDOWS)
    set(ODB_URL "https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/libodb-2.5.0-x64-windows-prebuilt.tar")
endif()

vcpkg_download_distfile(ARCHIVE
    URLS ${ODB_URL}
    FILENAME "libodb-2.5.0.tar"
    SHA512 c900d6dbe371fc98354449bf6b373ca9c1629c126338bcb99bf4047b5a3fb7272b703903ec64fd66e4af9e1b1de136ebfaa89df1733ab28a7f58a23f193e745e
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
# https://codesynthesis.com/download/odb/2.5.0/windows/windows10/x86_64/libodb-2.5.0-x86_64-windows10-msvc17.10-debug.zip
# https://codesynthesis.com/download/odb/2.5.0/windows/windows10/x86_64/libodb-2.5.0-x86_64-windows10-msvc17.10.zip

# odb 2.5.0
# https://codesynthesis.com/download/odb/2.5.0/windows/windows10/x86_64/odb-2.5.0-x86_64-windows10.zip

# NOTE: These binaries have got a problem with there debug naming or something. I couldn't fix it at the time but I'll, soon.
#       There is no libodb 2.5.0 in vcpkg as of now.