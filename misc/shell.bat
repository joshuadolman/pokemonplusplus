@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set path=w:\pokemonplusplus\misc;%path%
set path=w:\4coder;%path%
set path=w:\pokemonplusplus\source;w:\pokemonplusplus\build;%path%
w:
cd pokemonplusplus