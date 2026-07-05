@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d D:\workspace\DevilutionX
cmake --build build_vs18 --target spell_tooltip_test -j
