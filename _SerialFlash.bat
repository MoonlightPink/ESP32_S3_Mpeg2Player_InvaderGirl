title _SerialFlash.bat
rmdir /s /q %temp%\arduino
set name=ESP32_S3_Mpeg2Player
set fqbn=esp32:esp32:esp32s3:EraseFlash=all,UploadSpeed=921600,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi
set com=5
echo off
:loop

:Compile
cls
echo Compiling...
del %name%.ino.bin
del %name%.ino.*.bin
..\arduino-cli.exe compile --library libs --library libmpeg2 --fqbn %fqbn% --output-dir . %name%.ino
del %name%.ino.elf
del %name%.ino.map

if not exist %name%.ino.bin goto CompileError

:Upload
echo Uploading...
..\arduino-cli.exe upload --port com%com% --fqbn %fqbn% %name%.ino
if %errorlevel% equ 0 goto loop
echo Error level: %errorlevel%
goto Upload

:CompileError
echo Compile error.
pause
goto Compile

