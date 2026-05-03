#pragma once

typedef long    int64;
typedef int     int32;
typedef short   int16;
typedef char    int8;

typedef unsigned long   uint64;
typedef unsigned int    uint32;
typedef unsigned short  uint16;
typedef unsigned char   uint8;

typedef uint64 ms;

#define KB(x) (uint64)1024
#define MB(x) (uint64)1024 * KB(x)
#define GB(x) (uint64)1024 * MB(x)

