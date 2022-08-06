#ifdef _MSC_VER
#define PACKED
#pragma pack(push, 1)
#else
#define PACKED __attribute__ ((__packed__))
#endif