// include/d3d8_to_gles.h
#ifndef D3D8_TO_GLES_H
#define D3D8_TO_GLES_H

#include "GLES/gl.h"
#include "GLES/glext.h"
#include "EGL/egl.h"
#include <stdint.h>
#include <d3d8types.h>
#include <d3d8caps.h>
#include <d3dx8math.h>

#define D3DAPI // Placeholder for WINAPI
#define D3D_SDK_VERSION 220
#define D3DX_DEFAULT ULONG_MAX
#define D3DX_DEFAULT_FLOAT FLT_MAX

// Error codes
#define D3D_OK 0
#define D3DERR_INVALIDCALL -1
#define MAKE_D3DHRESULT(code) (0x88760000 | (code))
#define D3DERR_OUTOFVIDEOMEMORY MAKE_D3DHRESULT(380)
#define D3DERR_NOTAVAILABLE MAKE_D3DHRESULT(2154)
#define D3DXERR_NOTAVAILABLE MAKE_DDHRESULT(2154)
#ifndef D3DXERR_INVALIDMESH
#define D3DXERR_INVALIDMESH MAKE_DDHRESULT(2901)
#endif
#ifndef D3DXERR_SKINNINGNOTSUPPORTED
#define D3DXERR_SKINNINGNOTSUPPORTED MAKE_DDHRESULT(2903)
#endif

#ifndef D3DADAPTER_DEFAULT
#define D3DADAPTER_DEFAULT 0
#endif

#ifndef D3DXMESH_MANAGED
#define D3DXMESH_MANAGED 0x220
#endif

#ifndef D3DXMESHOPT_COMPACT
#define D3DXMESHOPT_COMPACT 0x01000000
#endif
#ifndef D3DXMESHOPT_ATTRSORT
#define D3DXMESHOPT_ATTRSORT 0x02000000
#endif
#ifndef D3DXMESHOPT_VERTEXCACHE
#define D3DXMESHOPT_VERTEXCACHE 0x04000000
#endif
#ifndef D3DXMESHOPT_STRIPREORDER
#define D3DXMESHOPT_STRIPREORDER 0x08000000
#endif
#ifndef D3DXMESHOPT_IGNOREVERTS
#define D3DXMESHOPT_IGNOREVERTS 0x10000000
#endif
#ifndef D3DXMESHOPT_SHAREVB
#define D3DXMESHOPT_SHAREVB 0x1000
#endif

// Minimal mesh definitions
#define D3DXMESH_MANAGED 0x220
#define MAX_FVF_DECL_SIZE 20

typedef struct _D3DXATTRIBUTERANGE {
    DWORD AttribId;
    DWORD FaceStart;
    DWORD FaceCount;
    DWORD VertexStart;
    DWORD VertexCount;
} D3DXATTRIBUTERANGE;

typedef D3DXATTRIBUTERANGE *LPD3DXATTRIBUTERANGE;

// Forward declarations
typedef struct IDirect3D8 IDirect3D8;
typedef struct IDirect3DDevice8 IDirect3DDevice8;
typedef struct IDirect3DVertexBuffer8 IDirect3DVertexBuffer8;
typedef struct IDirect3DIndexBuffer8 IDirect3DIndexBuffer8;
typedef struct ID3DXBuffer ID3DXBuffer;
typedef struct ID3DXMesh ID3DXMesh;
typedef struct ID3DXMatrixStack ID3DXMatrixStack;

// Internal state structure
typedef struct {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    GLboolean depth_test;
    GLboolean cull_face;
    GLenum cull_mode;
    GLboolean blend;
    GLfloat ambient[4];
    GLuint current_vbo;
    GLuint current_ibo;
    D3DXMATRIX world_matrix;
    D3DXMATRIX view_matrix;
    D3DXMATRIX projection_matrix;
    D3DVIEWPORT8 viewport;
    DWORD fvf;
    DWORD attrib_id;
} GLES_Device;

// Vertex/index buffer structure
typedef struct {
    GLuint vbo_id;
    UINT length;
    DWORD usage;
    DWORD fvf;
    D3DFORMAT format;
    D3DPOOL pool;
    BYTE *temp_buffer;
} GLES_Buffer;

// ID3DXBuffer interface
typedef struct {
    HRESULT (D3DAPI *QueryInterface)(ID3DXBuffer *This, REFIID iid, void **ppv);
    ULONG (D3DAPI *AddRef)(ID3DXBuffer *This);
    ULONG (D3DAPI *Release)(ID3DXBuffer *This);
    LPVOID (*GetBufferPointer)(ID3DXBuffer *This);
    DWORD (*GetBufferSize)(ID3DXBuffer *This);
} ID3DXBufferVtbl;

struct ID3DXBuffer {
    const ID3DXBufferVtbl *pVtbl;
    void *data;
    DWORD size;
};

// ID3DXMatrixStack interface
typedef struct {
    HRESULT (D3DAPI *QueryInterface)(ID3DXMatrixStack *This, REFIID riid, LPVOID *ppvObj);
    ULONG (D3DAPI *AddRef)(ID3DXMatrixStack *This);
    ULONG (D3DAPI *Release)(ID3DXMatrixStack *This);
    HRESULT (D3DAPI *Pop)(ID3DXMatrixStack *This);
    HRESULT (D3DAPI *Push)(ID3DXMatrixStack *This);
    HRESULT (D3DAPI *LoadIdentity)(ID3DXMatrixStack *This);
    HRESULT (D3DAPI *LoadMatrix)(ID3DXMatrixStack *This, CONST D3DXMATRIX *pM);
    HRESULT (D3DAPI *MultMatrix)(ID3DXMatrixStack *This, CONST D3DXMATRIX *pM);
    HRESULT (D3DAPI *MultMatrixLocal)(ID3DXMatrixStack *This, CONST D3DXMATRIX *pM);
    HRESULT (D3DAPI *RotateAxis)(ID3DXMatrixStack *This, CONST D3DXVECTOR3 *pV, FLOAT Angle);
    HRESULT (D3DAPI *RotateAxisLocal)(ID3DXMatrixStack *This, CONST D3DXVECTOR3 *pV, FLOAT Angle);
    HRESULT (D3DAPI *RotateYawPitchRoll)(ID3DXMatrixStack *This, FLOAT Yaw, FLOAT Pitch, FLOAT Roll);
    HRESULT (D3DAPI *RotateYawPitchRollLocal)(ID3DXMatrixStack *This, FLOAT Yaw, FLOAT Pitch, FLOAT Roll);
    HRESULT (D3DAPI *Scale)(ID3DXMatrixStack *This, FLOAT x, FLOAT y, FLOAT z);
    HRESULT (D3DAPI *ScaleLocal)(ID3DXMatrixStack *This, FLOAT x, FLOAT y, FLOAT z);
    HRESULT (D3DAPI *Translate)(ID3DXMatrixStack *This, FLOAT x, FLOAT y, FLOAT z);
    HRESULT (D3DAPI *TranslateLocal)(ID3DXMatrixStack *This, FLOAT x, FLOAT y, FLOAT z);
    D3DXMATRIX* (*GetTop)(ID3DXMatrixStack *This);
} ID3DXMatrixStackVtbl;

struct ID3DXMatrixStack {
    const ID3DXMatrixStackVtbl *pVtbl;
    D3DXMATRIX *stack;
    DWORD capacity;
    DWORD top;
};

// ID3DXMesh interface (full, from d3dx8mesh.h)
#undef INTERFACE
#define INTERFACE ID3DXMesh
DECLARE_INTERFACE_(ID3DXMesh, ID3DXBaseMesh)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(DrawSubset)(THIS_ DWORD AttribId) PURE;
    STDMETHOD_(DWORD, GetNumFaces)(THIS) PURE;
    STDMETHOD_(DWORD, GetNumVertices)(THIS) PURE;
    STDMETHOD_(DWORD, GetFVF)(THIS) PURE;
    STDMETHOD(GetDeclaration)(THIS_ DWORD Declaration[MAX_FVF_DECL_SIZE]) PURE;
    STDMETHOD_(DWORD, GetOptions)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE8 *ppDevice) PURE;
    STDMETHOD(CloneMeshFVF)(THIS_ DWORD Options, DWORD FVF, LPDIRECT3DDEVICE8 pD3DDevice, LPD3DXMESH *ppCloneMesh) PURE;
    STDMETHOD(CloneMesh)(THIS_ DWORD Options, CONST DWORD *pDeclaration, LPDIRECT3DDEVICE8 pD3DDevice, LPD3DXMESH *ppCloneMesh) PURE;
    STDMETHOD(GetVertexBuffer)(THIS_ LPDIRECT3DVERTEXBUFFER8 *ppVB) PURE;
    STDMETHOD(GetIndexBuffer)(THIS_ LPDIRECT3DINDEXBUFFER8 *ppIB) PURE;
    STDMETHOD(LockVertexBuffer)(THIS_ DWORD Flags, BYTE **ppData) PURE;
    STDMETHOD(UnlockVertexBuffer)(THIS) PURE;
    STDMETHOD(LockIndexBuffer)(THIS_ DWORD Flags, BYTE **ppData) PURE;
    STDMETHOD(UnlockIndexBuffer)(THIS) PURE;
    STDMETHOD(GetAttributeTable)(THIS_ D3DXATTRIBUTERANGE *pAttribTable, DWORD *pAttribTableSize) PURE;
    STDMETHOD(ConvertPointRepsToAdjacency)(THIS_ CONST DWORD *pPRep, DWORD *pAdjacency) PURE;
    STDMETHOD(ConvertAdjacencyToPointReps)(THIS_ CONST DWORD *pAdjacency, DWORD *pPRep) PURE;
    STDMETHOD(GenerateAdjacency)(THIS_ FLOAT Epsilon, DWORD *pAdjacency) PURE;
    STDMETHOD(LockAttributeBuffer)(THIS_ DWORD Flags, DWORD **ppData) PURE;
    STDMETHOD(UnlockAttributeBuffer)(THIS) PURE;
    STDMETHOD(Optimize)(THIS_ DWORD Flags, CONST DWORD *pAdjacencyIn, DWORD *pAdjacencyOut, DWORD *pFaceRemap, LPD3DXBUFFER *ppVertexRemap, LPD3DXMESH *ppOptMesh) PURE;
    STDMETHOD(OptimizeInplace)(THIS_ DWORD Flags, CONST DWORD *pAdjacencyIn, DWORD *pAdjacencyOut, DWORD *pFaceRemap, LPD3DXBUFFER *ppVertexRemap) PURE;
};

struct ID3DXMesh {
    const ID3DXMeshVtbl *pVtbl;
    IDirect3DDevice8 *device;
    IDirect3DVertexBuffer8 *vb;
    IDirect3DIndexBuffer8 *ib;
    DWORD num_vertices;
    DWORD num_faces;
    DWORD fvf;
    DWORD options;
    D3DXATTRIBUTERANGE *attrib_table;
    DWORD attrib_table_size;
    DWORD *attrib_buffer;
};

// IDirect3D8 interface
typedef struct {
    HRESULT (D3DAPI *QueryInterface)(IDirect3D8 *This, REFIID riid, void **ppvObj);
    ULONG (D3DAPI *AddRef)(IDirect3D8 *This);
    ULONG (D3DAPI *Release)(IDirect3D8 *This);
    HRESULT (D3DAPI *RegisterSoftwareDevice)(IDirect3D8 *This, void *pInitializeFunction);
    UINT (D3DAPI *GetAdapterCount)(IDirect3D8 *This);
    HRESULT (D3DAPI *GetAdapterIdentifier)(IDirect3D8 *This, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8 *pIdentifier);
    UINT (D3DAPI *GetAdapterModeCount)(IDirect3D8 *This, UINT Adapter);
    HRESULT (D3DAPI *EnumAdapterModes)(IDirect3D8 *This, UINT Adapter, UINT Mode, D3DDISPLAYMODE *pMode);
    HRESULT (D3DAPI *GetAdapterDisplayMode)(IDirect3D8 *This, UINT Adapter, D3DDISPLAYMODE *pMode);
    HRESULT (D3DAPI *CheckDeviceType)(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed);
    HRESULT (D3DAPI *CheckDeviceFormat)(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat);
    HRESULT (D3DAPI *CheckDeviceMultiSampleType)(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType);
    HRESULT (D3DAPI *CheckDepthStencilMatch)(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat);
    HRESULT (D3DAPI *GetDeviceCaps)(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8 *pCaps);
    HMONITOR (D3DAPI *GetAdapterMonitor)(IDirect3D8 *This, UINT Adapter);
    HRESULT (D3DAPI *CreateDevice)(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice8 **ppReturnedDeviceInterface);
} IDirect3D8Vtbl;

struct IDirect3D8 {
    const IDirect3D8Vtbl *lpVtbl;
};

// IDirect3DDevice8 interface
typedef struct {
    HRESULT (D3DAPI *QueryInterface)(IDirect3DDevice8 *This, REFIID riid, void **ppvObj);
    ULONG (D3DAPI *AddRef)(IDirect3DDevice8 *This);
    ULONG (D3DAPI *Release)(IDirect3DDevice8 *This);
    HRESULT (D3DAPI *TestCooperativeLevel)(IDirect3DDevice8 *This);
    UINT (D3DAPI *GetAvailableTextureMem)(IDirect3DDevice8 *This);
    HRESULT (D3DAPI *ResourceManagerDiscardBytes)(IDirect3DDevice8 *This, DWORD Bytes);
    HRESULT (D3DAPI *GetDirect3D)(IDirect3DDevice8 *This, IDirect3D8 **ppD3D8);
    HRESULT (D3DAPI *GetDeviceCaps)(IDirect3DDevice8 *This, D3DCAPS8 *pCaps);
    HRESULT (D3DAPI *GetDisplayMode)(IDirect3DDevice8 *This, D3DDISPLAYMODE *pMode);
    HRESULT (D3DAPI *GetCreationParameters)(IDirect3DDevice8 *This, D3DDEVICE_CREATION_PARAMETERS *pParameters);
    HRESULT (D3DAPI *SetCursorProperties)(IDirect3DDevice8 *This, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8 *pCursorBitmap);
    void (D3DAPI *SetCursorPosition)(IDirect3DDevice8 *This, UINT XScreenSpace, UINT YScreenSpace, DWORD Flags);
    BOOL (D3DAPI *ShowCursor)(IDirect3DDevice8 *This, BOOL bShow);
    HRESULT (D3DAPI *CreateAdditionalSwapChain)(IDirect3DDevice8 *This, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DSwapChain8 **pSwapChain);
    HRESULT (D3DAPI *Reset)(IDirect3DDevice8 *This, D3DPRESENT_PARAMETERS *pPresentationParameters);
    HRESULT (D3DAPI *Present)(IDirect3DDevice8 *This, CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion);
    HRESULT (D3DAPI *GetBackBuffer)(IDirect3DDevice8 *This, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8 **ppBackBuffer);
    HRESULT (D3DAPI *GetRasterStatus)(IDirect3DDevice8 *This, D3DRASTER_STATUS *pRasterStatus);
    void (D3DAPI *SetGammaRamp)(IDirect3DDevice8 *This, DWORD Flags, CONST D3DGAMMARAMP *pRamp);
    void (D3DAPI *GetGammaRamp)(IDirect3DDevice8 *This, D3DGAMMARAMP *pRamp);
    HRESULT (D3DAPI *CreateVertexBuffer)(IDirect3DDevice8 *This, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8 **ppVertexBuffer);
    HRESULT (D3DAPI *CreateIndexBuffer)(IDirect3DDevice8 *This, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8 **ppIndexBuffer);
    HRESULT (D3DAPI *SetRenderState)(IDirect3DDevice8 *This, D3DRENDERSTATETYPE State, DWORD Value);
    HRESULT (D3DAPI *BeginScene)(IDirect3DDevice8 *This);
    HRESULT (D3DAPI *EndScene)(IDirect3DDevice8 *This);
    HRESULT (D3DAPI *SetStreamSource)(IDirect3DDevice8 *This, UINT StreamNumber, IDirect3DVertexBuffer8 *pStreamData, UINT Stride);
    HRESULT (D3DAPI *SetIndices)(IDirect3DDevice8 *This, IDirect3DIndexBuffer8 *pIndexData, UINT BaseVertexIndex);
    HRESULT (D3DAPI *SetViewport)(IDirect3DDevice8 *This, CONST D3DVIEWPORT8 *pViewport);
    HRESULT (D3DAPI *SetTransform)(IDirect3DDevice8 *This, D3DTRANSFORMSTATETYPE State, CONST D3DXMATRIX *pMatrix);
    HRESULT (D3DAPI *DrawIndexedPrimitive)(IDirect3DDevice8 *This, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);
} IDirect3DDevice8Vtbl;

struct IDirect3DDevice8 {
    const IDirect3DDevice8Vtbl *lpVtbl;
    GLES_Device *gles;
    IDirect3D8 *d3d8;
};

// IDirect3DVertexBuffer8 interface
typedef struct {
    HRESULT (D3DAPI *QueryInterface)(IDirect3DVertexBuffer8 *This, REFIID riid, void **ppvObj);
    ULONG (D3DAPI *AddRef)(IDirect3DVertexBuffer8 *This);
    ULONG (D3DAPI *Release)(IDirect3DVertexBuffer8 *This);
    HRESULT (D3DAPI *GetDevice)(IDirect3DVertexBuffer8 *This, IDirect3DDevice8 **ppDevice);
    HRESULT (D3DAPI *SetPrivateData)(IDirect3DVertexBuffer8 *This, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags);
    HRESULT (D3DAPI *GetPrivateData)(IDirect3DVertexBuffer8 *This, REFGUID refguid, void *pData, DWORD *pSizeOfData);
    HRESULT (D3DAPI *FreePrivateData)(IDirect3DVertexBuffer8 *This, REFGUID refguid);
    DWORD (D3DAPI *SetPriority)(IDirect3DVertexBuffer8 *This, DWORD PriorityNew);
    DWORD (D3DAPI *GetPriority)(IDirect3DVertexBuffer8 *This);
    void (D3DAPI *PreLoad)(IDirect3DVertexBuffer8 *This);
    D3DRESOURCETYPE (D3DAPI *GetType)(IDirect3DVertexBuffer8 *This);
    HRESULT (D3DAPI *Lock)(IDirect3DVertexBuffer8 *This, UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags);
    HRESULT (D3DAPI *Unlock)(IDirect3DVertexBuffer8 *This);
    HRESULT (D3DAPI *GetDesc)(IDirect3DVertexBuffer8 *This, D3DVERTEXBUFFER_DESC *pDesc);
} IDirect3DVertexBuffer8Vtbl;

struct IDirect3DVertexBuffer8 {
    const IDirect3DVertexBuffer8Vtbl *lpVtbl;
    GLES_Buffer *buffer;
    IDirect3DDevice8 *device;
};

// IDirect3DIndexBuffer8 interface
typedef struct {
    HRESULT (D3DAPI *QueryInterface)(IDirect3DIndexBuffer8 *This, REFIID riid, void **ppvObj);
    ULONG (D3DAPI *AddRef)(IDirect3DIndexBuffer8 *This);
    ULONG (D3DAPI *Release)(IDirect3DIndexBuffer8 *This);
    HRESULT (D3DAPI *GetDevice)(IDirect3DIndexBuffer8 *This, IDirect3DDevice8 **ppDevice);
    HRESULT (D3DAPI *SetPrivateData)(IDirect3DIndexBuffer8 *This, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags);
    HRESULT (D3DAPI *GetPrivateData)(IDirect3DIndexBuffer8 *This, REFGUID refguid, void *pData, DWORD *pSizeOfData);
    HRESULT (D3DAPI *FreePrivateData)(IDirect3DIndexBuffer8 *This, REFGUID refguid);
    DWORD (D3DAPI *SetPriority)(IDirect3DIndexBuffer8 *This, DWORD PriorityNew);
    DWORD (D3DAPI *GetPriority)(IDirect3DIndexBuffer8 *This);
    void (D3DAPI *PreLoad)(IDirect3DIndexBuffer8 *This);
    D3DRESOURCETYPE (D3DAPI *GetType)(IDirect3DIndexBuffer8 *This);
    HRESULT (D3DAPI *Lock)(IDirect3DIndexBuffer8 *This, UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags);
    HRESULT (D3DAPI *Unlock)(IDirect3DIndexBuffer8 *This);
    HRESULT (D3DAPI *GetDesc)(IDirect3DIndexBuffer8 *This, D3DINDEXBUFFER_DESC *pDesc);
} IDirect3DIndexBuffer8Vtbl;

struct IDirect3DIndexBuffer8 {
    const IDirect3DIndexBuffer8Vtbl *lpVtbl;
    GLES_Buffer *buffer;
    IDirect3DDevice8 *device;
};

// D3DX function prototypes
HRESULT WINAPI D3DXCreateBox(LPDIRECT3DDEVICE8 pDevice, FLOAT Width, FLOAT Height, FLOAT Depth, LPD3DXMESH *ppMesh, LPD3DXBUFFER *ppAdjacency);
HRESULT WINAPI D3DXCreateBuffer(DWORD NumBytes, LPD3DXBUFFER *ppBuffer);
HRESULT WINAPI D3DXGetErrorStringA(HRESULT hr, LPSTR pBuffer, UINT BufferLen);
HRESULT WINAPI D3DXCreateMatrixStack(DWORD Flags, LPD3DXMATRIXSTACK *ppStack);

// Math functions
D3DXMATRIX* WINAPI D3DXMatrixIdentity(D3DXMATRIX *pOut);
D3DXMATRIX* WINAPI D3DXMatrixMultiply(D3DXMATRIX *pOut, CONST D3DXMATRIX *pM1, CONST D3DXMATRIX *pM2);
D3DXMATRIX* WINAPI D3DXMatrixLookAtLH(D3DXMATRIX *pOut, CONST D3DXVECTOR3 *pEye, CONST D3DXVECTOR3 *pAt, CONST D3DXVECTOR3 *pUp);
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovLH(D3DXMATRIX *pOut, FLOAT fovy, FLOAT Aspect, FLOAT zn, FLOAT zf);
D3DXVECTOR3* WINAPI D3DXVec3Normalize(D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV);
D3DXVECTOR3* WINAPI D3DXVec3TransformCoord(D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM);

// Entry point
IDirect3D8 *D3DAPI Direct3DCreate8(UINT SDKVersion);

#ifdef D3D8_GLES_LOGGING
void d3d8_gles_log(const char *format, ...);
#else
#define d3d8_gles_log(...)
#endif

#endif // D3D8_TO_GLES_H