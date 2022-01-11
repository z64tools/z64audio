# z64audio
Somewhat flexible audio converter.

## Input / Output
Input formats: `WAV, AIFF, AIFC` \
Output formats: `WAV, AIFF, BIN, C`

## Arguments
```
File:
-i [file]         // Input:  .wav .aiff .aifc
-o [file]         // Output: .wav .aiff .bin .c
-c                // Compare I [file] & O [file]

Audio Processing:
-b [ 16 ]         // Target Bit Depth
-m                // Mono
-n                // Normalize

VADPCM:
-p [file]         // Use excisting predictors, does not run TableDesign
-I [ 30 ]         // TableDesign Refine Iteration
-F [ 16 ]         // TableDesign Frame Size
-B [  2 ]         // TableDesign Bits
-O [  2 ]         // TableDesign Order
-T [ 10 ]         // TableDesign Threshold

Extra:
-D                // Debug Print
-s                // Silence
-N                // Print Info of input [file]
```
## Credits
`z64me`: helping on various things \
`sm64 decomp`: sdk audio tools `tabledesign` `vadpcm_enc` and `vadpcm_dec`
