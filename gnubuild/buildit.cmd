@echo off
@rem Set the device you want to build for to 1
@rem Use Pixi to allow running on either device
set PRE=1
set PIXI=0
set DEBUG=0

@rem List your source files here
set SRC=..\control\file.cpp ..\control\map.cpp ..\control\render.cpp ..\control\repository.cpp ..\control\tile.cpp ..\pdk\main.cpp

@rem List the libraries needed
set LIBS=-lSDL_image -lSDL -lGLES_CM -lpdl

@rem Name your output executable
set OUTFILE=lindsay_plugin

if %PRE% equ 0 if %PIXI% equ 0 goto :END

if %DEBUG% equ 1 (
   set DEVICEOPTS=-g
) else (
   set DEVICEOPTS=
)

if %PRE% equ 1 (
   set DEVICEOPTS=%DEVICEOPTS% -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
)

if %PIXI% equ 1 (
   set DEVICEOPTS=%DEVICEOPTS% -mcpu=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp
)

set LINDSAYINC="-I..\control" "-I..\control_pdk"
set DEVICEOPTS=%DEVICEOPTS% -std=c++0x

echo %DEVICEOPTS%

arm-none-linux-gnueabi-gcc %DEVICEOPTS% -o %OUTFILE% %SRC% %LINDSAYINC% "-I%PALMPDK%\include" "-I%PALMPDK%\include\SDL" "-L%PALMPDK%\device\lib" -Wl,--allow-shlib-undefined %LIBS%

goto :EOF

:END
echo Please select the target device by editing the PRE/PIXI variable in this file.
exit /b 1

