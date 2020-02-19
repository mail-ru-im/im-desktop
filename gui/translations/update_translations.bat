@echo off
set LIST_LANGS=(ru)

for %%i in %LIST_LANGS% do (
	call ..\..\external\qt\windows\bin\lupdate.exe .. ..\..\gui.shared -ts %%i.ts -noobsolete -source-language en -target-language %%i -locations none	
)