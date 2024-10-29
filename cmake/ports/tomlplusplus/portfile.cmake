vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO marzer/tomlplusplus
    REF v3.4.0
    SHA512 c227fc8147c9459b29ad24002aaf6ab2c42fac22ea04c1c52b283a0172581ccd4527b33c1931e0ef0d1db6b6a53f9e9882c6d4231c7f3494cf070d0220741aa5
    HEAD_REF "v${VERSION}"
)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/tomlplusplus)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
# this is created but useless as the library is header only, so we get a warning
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
