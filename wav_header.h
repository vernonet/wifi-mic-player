#ifndef WAV_HEADER_H
#define WAV_HEADER_H

// Typedefs for the wave file
typedef char ID[4];

// Record whether we have found a format chunk yet
 int haveValidFormat = false;

//-----------------------------------
// Define structures used in WAV files

// Stores the file header information
typedef struct
{
    ID     groupID;
    long   size;
    ID     riffType;
} WAVRIFFHEADER;

// Stores the format information for the file
typedef struct {
    ID             chunkID;
    long           chunkSize;
    //int pos = FTell();
    short          wFormatTag;
    unsigned short wChannels;
    unsigned long  dwSamplesPerSec;
    unsigned long  dwAvgBytesPerSec;
    unsigned short wBlockAlign;
    unsigned short wBitsPerSample;
} FORMATCHUNK;

typedef struct
{
    ID             chunkID;
    long           chunkSize;
} DATACHUNK;




#endif // WAV_HEADER_H
