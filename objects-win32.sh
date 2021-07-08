# clean up current directory
rm *.o

# compile C++ objects using g++
# we request c++11 features because llrint etc are used in this code
~/c/mxe/usr/bin/i686-w64-mingw32.static-g++ -std=c++11 -c -Ofast -s -DNDEBUG tools/audiofile/audiofile.cpp -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE -municode -DUNICODE -D_UNICODE

# compile C objects using gcc
~/c/mxe/usr/bin/i686-w64-mingw32.static-gcc -c tools/sdk-tools/tabledesign/*.c -Ofast -s -DNDEBUG -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE -municode -DUNICODE -D_UNICODE

# compile C objects using gcc
~/c/mxe/usr/bin/i686-w64-mingw32.static-gcc -c \
	tools/sdk-tools/adpcm/quant.c \
	tools/sdk-tools/adpcm/sampleio.c \
	tools/sdk-tools/adpcm/util.c \
	tools/sdk-tools/adpcm/vpredictor.c \
	tools/sdk-tools/adpcm/vadpcm_enc.c \
	tools/sdk-tools/adpcm/vencode.c \
	-Ofast -s -DNDEBUG -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE -municode -DUNICODE -D_UNICODE

# move objects into appropriate directory
mkdir -p bin/o/win32
mv *.o bin/o/win32


