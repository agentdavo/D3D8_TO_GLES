#ifndef OBJBASE_H
#define OBJBASE_H

#ifndef COM_NO_WINDOWS_H
#include <windows.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef interface
#define interface struct
#endif

typedef struct _GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} GUID;

typedef GUID IID;
typedef GUID CLSID;
typedef const IID *REFIID;
typedef const CLSID *REFCLSID;
typedef const GUID *REFGUID;

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

#ifndef DEFINE_GUID
#ifdef INITGUID
#define DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
    EXTERN_C const GUID name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}
#else
#define DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
    EXTERN_C const GUID name
#endif
#endif

#define STDMETHOD(method) HRESULT (WINAPI *method)
#define STDMETHOD_(type,method) type (WINAPI *method)
#define STDMETHODIMP HRESULT WINAPI
#define STDMETHODIMP_(type) type WINAPI
#define PURE
#define THIS void *This
#define THIS_ void *This,

#define DECLARE_INTERFACE_(iface, baseiface) \
    typedef struct iface iface; \
    typedef struct iface##Vtbl iface##Vtbl; \
    struct iface##Vtbl
#define DECLARE_INTERFACE(iface) DECLARE_INTERFACE_(iface, IUnknown)

typedef interface IUnknown IUnknown;
struct IUnknownVtbl {
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
};
interface IUnknown {
    const struct IUnknownVtbl *lpVtbl;
};

// Minimal IStream forward declaration for d3dx8 headers
typedef interface IStream IStream;

#ifdef __cplusplus
}
#endif

#endif // OBJBASE_H
