# clean up current directory
rm *.o

# compile C++ objects using g++
# we request c++11 features because llrint etc are used in this code
g++ -std=c++11 -c -s -Ofast -DNDEBUG tools/audiofile/audiofile.cpp -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE

# compile C objects using gcc
gcc -c tools/sdk-tools/tabledesign/*.c -s -Ofast -DNDEBUG -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE

# compile C objects using gcc
gcc -c \
	tools/sdk-tools/adpcm/quant.c \
	tools/sdk-tools/adpcm/sampleio.c \
	tools/sdk-tools/adpcm/util.c \
	tools/sdk-tools/adpcm/vpredictor.c \
	tools/sdk-tools/adpcm/vadpcm_enc.c \
	tools/sdk-tools/adpcm/vencode.c \
	-s -Ofast -DNDEBUG -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE

# move objects into appropriate directory
mkdir -p bin/o/linux
mv *.o bin/o/linux


