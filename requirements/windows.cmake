
message(STATUS "")
message(STATUS "[CMAKE]")
message(STATUS "[CMAKE] including <requirements/windows.cmake>")
message(STATUS "[CMAKE]")


## Library packages
# depName depVersion depLinking depArch depHeaders depCommit depEnvironment depUrl
common_add_ext_package("boost" "1.72.0-5" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("curl" "7.61.1-1" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("ffmpeg" "4.0.2-5" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("libphonenumber" "8.12.11-5" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("nghttp2" "1.41.0-5" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("openal" "1.20.1-5" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("openssl" "1.1.1f-5" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("protobuf" "3.3.2-6" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("re2" "2020-10-01-5" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("zstd" "1.4.5-5" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("rlottie" "master-windows-static-5" "static" "${PACKAGE_ARCH}" OFF "7c5b40c" "${PACKAGE_ENV}" OFF)
common_add_ext_package("runtime" "10.0.19041.0" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("angle" "master-2021-05-24" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("7zip" "master" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)
common_add_ext_package("mesa3d" "21.1.3-1" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
if(IM_QT_DYNAMIC)
    common_add_ext_package("qt" "5.15-8" "dynamic" "${PACKAGE_ARCH}" "5.15.3" OFF "release" OFF)
else()
    common_add_ext_package("qt" "5.15-8" "static" "${PACKAGE_ARCH}" "5.15.2" OFF "${PACKAGE_ENV}" OFF)
endif()
# installer specific
common_add_ext_package("installer_qt" "5.15.2-1" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("installer_nghttp2" "1.41.0-1" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)
common_add_ext_package("installer_curl" "7.62.0-1" "static" "${PACKAGE_ARCH}" OFF OFF "release" OFF)

# Just includes for windows, use precompiled binary
common_add_ext_package("zlib" "1.2.11-6" "static" "${PACKAGE_ARCH}" OFF OFF "${PACKAGE_ENV}" OFF)

## Source packages
# depName depVersion depCommit depUrl depHash
common_add_ext_srcpkg("rapidjson" "master" "0ccdbf36" OFF OFF)
common_add_ext_srcpkg("minizip" "1.0.1e" OFF OFF OFF)


## Binaries
# depName depVersion depUrl depHash
common_add_ext_binaries("zlib.lib" "v13" OFF OFF)
common_add_ext_binaries("lrelease.exe" "v13" OFF OFF)
common_add_ext_binaries("lupdate.exe" "v13" OFF OFF)
common_add_ext_binaries("d3dcompiler_47.dll" "v13" OFF OFF)
# common_add_ext_binaries("opengl32sw.dll" "v13" OFF OFF)

