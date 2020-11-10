@echo off
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -D_CRT_SECURE_NO_WARNINGS -diagnostics:column -WL -Od -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7 ..\handmade\dota\dotahero.cpp
popd
..\..\build\dotahero.exe
