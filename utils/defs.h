#ifndef _COMMON_DEFS_INC_
#define _COMMON_DEFS_INC_

#include <stdint.h>

class CBaseEntity;
class CBasePlayer;

#pragma warning (disable : 4005) //stupid warning
#pragma warning (disable : 4996) //stupid warning
#ifndef _DEBUG
#pragma warning (disable : 4530) //exceptions disabled
#endif

#define MSG_PREFIX "[CatConnect] "

#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __attribute__((always_inline))
#endif

#ifdef DEBUG
#define FASTERCALL
#else
#ifdef _WIN32
#define FASTERCALL __vectorcall
#elif defined __linux__
#ifdef __clang__
#if __clang_major__ > 7
//#define FASTERCALL __regcall //5000 iq compiler crash ( https://i.imgur.com/wmL6FRK.png )
#define FASTERCALL __fastcall
#elif __clang_major__ > 4
#define FASTERCALL __vectorcall
#else
#define FASTERCALL __fastcall
#endif
#elif defined __GNUC__
#define FASTERCALL __attribute__((fastcall))
#endif
#endif
#endif

#endif