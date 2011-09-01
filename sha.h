#define MSDOS
#define LITTLE_ENDIAN

typedef int                int32;
typedef unsigned int      uint32;

//typedef unsigned int       UINT4;
typedef unsigned short int UINT2;

/*
 * just enough declarations for otp_hash to deal.
 */ 
void sha_memory (char *, uint32, uint32 *);


