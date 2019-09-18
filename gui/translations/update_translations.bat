@echo off
set LIST_LANGS=(en ru de pt uk cs fr ar vi zh es tr)

for %%i in %LIST_LANGS% do (
	call ..\..\external\qt\windows\bin\lupdate.exe .. ..\..\gui.shared -ts %%i.ts -noobsolete -source-language en -target-language %%i -locations none	
)