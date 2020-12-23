#!/bin/sh

external_version=`cat ../../requirements/common.cmake |grep "set(EXT_LIBS_VERSION" |awk '{print $2}' |tr -d '")'`
echo "info -> external_version = ${external_version}"
echo "info -> ../../external_${external_version}/qt/macos/bin/lupdate"
echo

../../external_${external_version}/qt/macos/bin/lupdate .. ../../gui.shared -ts ru.ts -noobsolete -source-language en -target-language ru -locations none