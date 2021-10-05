
message(STATUS "")
message(STATUS "[CMAKE]")
message(STATUS "[CMAKE] including <requirements/linux.cmake>")
message(STATUS "[CMAKE]")


## Library packages
# depName depVersion depLinking depArch depHeaders depCommit depEnvironment depUrl
common_add_ext_package("boost" "1.68.0-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("breakpad" "master-on-centos" "static" "${PACKAGE_ARCH}" OFF "46f4b593" "release" OFF)
common_add_ext_package("curl" "7.62.0-3" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("ffmpeg" "4.3.1-3" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("libdbus" "1.10.24-3" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("libphonenumber" "8.12.11-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("nghttp2" "1.41.0-3" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("openal" "1.20.1-3" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("openssl" "1.1.1-3" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("protobuf" "3.3.2-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("re2" "2020-10-01-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("zlib" "1.2.11-3" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("zstd" "1.4.5-3" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("rlottie" "master-on-centos" "static" "${PACKAGE_ARCH}" OFF "29b391b" "release" OFF)
common_add_ext_package("libxau" "1.0.9-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("libxdmcp" "1.1.3" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("xcb" "1.14-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("xproto" "7.0.31-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("xcb-util" "0.4.0-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("xcb-util-wm" "0.4.1-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("xcb-util-image" "0.4.0-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("xcb-util-keysyms" "0.4.0-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("xcb-util-renderutil" "0.3.9-2" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("xkbcommon" "1.0.3-3" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("xkbcommon-x11" "1.0.3-3" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
if(IM_QT_DYNAMIC)
    common_add_ext_package("qt" "5.15-5" "dynamic" "${PACKAGE_ARCH}" "5.15.3" OFF "release" OFF)
else()
    common_add_ext_package("qt" "5.14.2" "static" "${PACKAGE_ARCH}" "5.14.2" OFF "release" OFF)
endif()


## Source packages
# depName depVersion depCommit depUrl depHash
common_add_ext_srcpkg("breakpad" "master-on-centos" "46f4b593" OFF OFF)
common_add_ext_srcpkg("rapidjson" "master" "0ccdbf36" OFF OFF)
common_add_ext_srcpkg("minizip" "1.0.1e" OFF OFF OFF)


## Binaries
# depName depVersion depUrl depHash
common_add_ext_binaries("libXext.a" "v13" OFF OFF)
common_add_ext_binaries("libXi.a" "v13" OFF OFF)
common_add_ext_binaries("lconvert" "v13" OFF OFF)
common_add_ext_binaries("lrelease" "v13" OFF OFF)
common_add_ext_binaries("moc" "v13" OFF OFF)
common_add_ext_binaries("qmake" "v13" OFF OFF)
common_add_ext_binaries("rcc" "v13" OFF OFF)
common_add_ext_binaries("uic" "v13" OFF OFF)

