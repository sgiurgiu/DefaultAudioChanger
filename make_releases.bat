CALL cd Release\
CALL "C:\Program Files\7-Zip\7z.exe" a ..\releases\DefaultAudioChanger.zip DefaultAudioChanger.exe LICENSE.txt -tzip

CALL cd ..
CALL cd x64\Release\
CALL "C:\Program Files\7-Zip\7z.exe" a ..\..\releases\DefaultAudioChanger_x64.zip DefaultAudioChanger.exe LICENSE.txt -tzip

call cd ..\..