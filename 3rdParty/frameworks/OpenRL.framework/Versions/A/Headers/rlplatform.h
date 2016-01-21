/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (c) 2006 - 2014 Caustic Graphics, Inc.    All rights reserved.        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __rlplatform_h_
#define __rlplatform_h_

#include <stddef.h>


/*-------------------------------------------------------------------------
 * Definition of RL_APICALL and RL_APIENTRY
 *-----------------------------------------------------------------------*/

#if defined(_WIN32) || defined(__VC32__)             /* Win32 */
#   if defined (_DLL_EXPORTS) && (_DLL_EXPORTS)
#       define RL_APICALL CAUSTIC_API
#   else
#       define RL_APICALL __declspec(dllimport)
#   endif
#elif defined (__ARMCC_VERSION)                      /* ADS */
#   define RL_APICALL
#elif defined (__SYMBIAN32__) && defined (__GCC32__) /* Symbian GCC */
#   define RL_APICALL __declspec(dllexport)
#elif defined (__GNUC__)                             /* GCC dependencies (kludge) */
#   define RL_APICALL __attribute__ ((visibility("default")))
#endif

#if !defined (RL_APICALL)
#   error Unsupported platform!
#endif

#define RL_APIENTRY


/*-------------------------------------------------------------------------
 * Platform-specific data type definitions
 *-----------------------------------------------------------------------*/

#if (defined(_WIN32) && (_MSC_VER))

typedef signed __int8    RLbyte;
typedef signed __int16   RLshort;
typedef signed __int32   RLint;
typedef signed __int64   RLlong;
typedef unsigned __int8  RLubyte;
typedef unsigned __int16 RLushort;
typedef unsigned __int32 RLuint;
typedef unsigned __int64 RLulong;

#ifdef _WIN64
typedef signed __int64   RLintptr;
#else
typedef signed __int32   RLintptr;
#endif

#else

#include <stdint.h>
#include <stdbool.h>

typedef int8_t           RLbyte;
typedef int16_t          RLshort;
typedef int32_t          RLint;
typedef int64_t          RLlong;
typedef uint8_t          RLubyte;
typedef uint16_t         RLushort;
typedef uint32_t         RLuint;
typedef uint64_t         RLulong;
typedef intptr_t         RLintptr;

#endif


#endif /* __rlplatform_h_ */
