#ifndef WINDOWS_H
#define WINDOWS_H

#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINAPI
#define WINAPI
#endif
#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef CONST
#define CONST const
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned long DWORD;
typedef float FLOAT;

typedef void *LPVOID;
typedef const void *LPCVOID;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef wchar_t WCHAR;
typedef WCHAR *LPWSTR;
typedef const WCHAR *LPCWSTR;

typedef void *HANDLE;
typedef HANDLE HINSTANCE;
typedef HANDLE HMODULE;

#ifndef HDC
typedef void *HDC;
#endif

#ifndef HFONT
typedef void *HFONT;
#endif

#ifndef PVOID
typedef void *PVOID;
#endif

#ifndef LOGFONT_DEFINED
typedef struct tagLOGFONT {
  int dummy;
} LOGFONT, *PLOGFONT, *LPLOGFONT;
#define LOGFONT_DEFINED
#endif

#ifndef GLYPHMETRICSFLOAT_DEFINED
typedef struct tagGLYPHMETRICSFLOAT {
  float dummy;
} GLYPHMETRICSFLOAT, *LPGLYPHMETRICSFLOAT;
#define GLYPHMETRICSFLOAT_DEFINED
#endif

typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t ULONG_PTR;

#define DECLARE_HANDLE(name)                                                   \
  typedef struct name##__ {                                                    \
    int unused;                                                                \
  } *name;
DECLARE_HANDLE(HWND);
#ifndef HMONITOR_DECLARED
#define HMONITOR_DECLARED
DECLARE_HANDLE(HMONITOR);
#endif

typedef struct tagRECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT, *PRECT, *LPRECT;

typedef struct _RGNDATA {
  char unused;
} RGNDATA, *PRGNDATA, *LPRGNDATA;

typedef LONG_PTR LPARAM;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LRESULT;

typedef long HRESULT;

#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#define FAILED(hr) ((HRESULT)(hr) < 0)

#define S_OK ((HRESULT)0L)
#define S_FALSE ((HRESULT)1L)
#define E_FAIL ((HRESULT)0x80004005L)
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)

#define MAKE_HRESULT(sev, fac, code)                                           \
  ((HRESULT)(((unsigned int)(sev) << 31) | ((unsigned int)(fac) << 16) |       \
             ((unsigned int)(code))))
#ifndef _FACDD
#define _FACDD 0x876
#endif
#define MAKE_DDHRESULT(code) MAKE_HRESULT(1, _FACDD, (code))

#ifdef __cplusplus
}
#endif

#endif // WINDOWS_H
