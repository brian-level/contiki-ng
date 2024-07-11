#!/bin/bash
level_platform=$1
arm-none-eabi-objcopy -O ihex ./build/${level_platform}/subghzcpu ./build/${level_platform}/subghzcpu.hex
echo "reset" > flashcmds.txt
#echo "erase" > flashcmds.txt
echo "loadfile ./build/${level_platform}/subghzcpu.hex" >> flashcmds.txt
echo "r" >> flashcmds.txt
echo "g" >> flashcmds.txt
echo "q" >> flashcmds.txt
JLinkExe -device EFR32ZG23BXXXF512 -speed 4000 -if SWD -autoconnect 1 -CommandFile flashcmds.txt
rm flashcmds.txt


