@echo off
 
pushd
cd w:\pokemonplusplus\build
cl /Zi w:\pokemonplusplus\source\pokemonpp.cpp User32.lib Gdi32.lib Dinput8.lib dxguid.lib
popd