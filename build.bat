@echo off

if not exist build mkdir build
pushd build

set CommonCompilerFlags=/Od /Oi /Z7 /Zo /MTd /fp:fast /fp:except- /nologo /FC /GR- /Gm- /EHa- /W4 /WX /wd4100 /wd4189 /wd4201 /wd4505 /DGAME_SLOW=1 /DGAME_INTERNAL=1
set CommonLinkerFlags=/opt:ref /incremental:no

del game.pdb 2> NUL

cl %CommonCompilerFlags% /DOUTPUT_BMP=0 /D_CRT_SECURE_NO_WARNINGS ..\atlas_builder.cpp /link %CommonLinkerFlags%

echo Building DLL... > lock.tmp
cl %CommonCompilerFlags% /Fmgame.map /LD ..\game.cpp /link %CommonLinkerFlags% /EXPORT:GameUpdateAndRender
del lock.tmp

cl %CommonCompilerFlags% ..\win32_game.cpp user32.lib gdi32.lib winmm.lib /link %CommonLinkerFlags%

popd

clang -mbulk-memory -Wno-writable-strings -DWASM_BUILD=1 -DGAME_SLOW=1 -DGAME_INTERNAL=1 --target=wasm32 -std=c++11 -nostdlib -O3 -Wl,--no-entry,--export=GameUpdateAndRender,--export=__heap_base,--import-memory -o game.wasm game.cpp
