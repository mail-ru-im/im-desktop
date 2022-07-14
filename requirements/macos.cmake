
message(STATUS "")
message(STATUS "[CMAKE]")
message(STATUS "[CMAKE] including <requirements/macos.cmake>")
message(STATUS "[CMAKE]")


## Library packages
# depName depVersion depLinking depArch depHeaders depCommit depEnvironment depUrl
common_add_ext_package("boost" "1.68.0-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("crashpad" "master-macos-static-5" "static" "${PACKAGE_ARCH}" OFF "6b55b8ad" "release" OFF)
common_add_ext_package("breakpad" "master-macos-static-5" "static" "${PACKAGE_ARCH}" OFF "dff7d5af" "release" OFF)
common_add_ext_package("curl" "7.62.0-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("ffmpeg" "4.3.1-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("libphonenumber" "8.12.11-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("nghttp2" "1.41.0-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("openal" "1.20.1-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("openssl" "1.1.1-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("protobuf" "3.3.2-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("re2" "2020-10-01-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("zlib" "1.2.11-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("zstd" "1.4.5-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("rlottie" "master-macos-static-5" "static" "${PACKAGE_ARCH}" OFF "29b391b" "release" OFF)
common_add_ext_package("sparkle" "1.26.0-5" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
if(IM_QT_DYNAMIC)
    common_add_ext_package("qt" "5.15-11" "dynamic" "${PACKAGE_ARCH}" "5.15.3" OFF "release" OFF)
else()
    common_add_ext_package("qt" "5.15-5" "static" "${PACKAGE_ARCH}" "5.15.3" OFF "release" OFF)
endif()


## Source packages
# depName depVersion depCommit depUrl depHash
common_add_ext_srcpkg("breakpad" "master-macos-static-5" "dff7d5af" OFF OFF)
common_add_ext_srcpkg("rapidjson" "master" "0ccdbf36" OFF OFF)
common_add_ext_srcpkg("minizip" "1.0.1e" OFF OFF OFF)


## Additonals
# depName depUrl depHash
common_add_additionals("macos_itunes" OFF OFF)
common_add_additionals("macos_protolib" OFF OFF)
common_add_additionals("macos_sskeychain" OFF OFF)
common_add_additionals("macos_xctest" OFF OFF)

