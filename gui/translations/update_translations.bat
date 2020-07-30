@ECHO OFF
SET InFile=..\..\CMakeLists.txt

FOR /F "tokens=*" %%A IN ('FINDSTR /N "set(EXT_LIBS_VERSION " "%InFile%"') DO (
    set EXT_VERSION=%%A
)
set STRIP1=%EXT_VERSION:*EXT_LIBS_VERSION =%
SET EXT_LIB_VERS=%STRIP1:)=%

set LIST_LANGS=(ru)

for %%i in %LIST_LANGS% do (
	call ..\..\external_%EXT_LIB_VERS%\qt\windows\bin\lupdate.exe .. ..\..\gui.shared -ts %%i.ts -noobsolete -source-language en -target-language %%i -locations none
)
