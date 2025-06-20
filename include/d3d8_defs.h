#ifndef D3D8_DEFS_H
#define D3D8_DEFS_H

#include "windows.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Basic enums */
typedef enum _D3DDEVTYPE {
    D3DDEVTYPE_HAL = 1,
    D3DDEVTYPE_REF = 2,
    D3DDEVTYPE_SW  = 3,
    D3DDEVTYPE_FORCE_DWORD = 0x7fffffff
} D3DDEVTYPE;

typedef enum _D3DMULTISAMPLE_TYPE {
    D3DMULTISAMPLE_NONE = 0,
    D3DMULTISAMPLE_2_SAMPLES = 2,
    D3DMULTISAMPLE_FORCE_DWORD = 0x7fffffff
} D3DMULTISAMPLE_TYPE;

typedef enum _D3DRESOURCETYPE {
    D3DRTYPE_SURFACE      = 1,
    D3DRTYPE_VOLUME       = 2,
    D3DRTYPE_TEXTURE      = 3,
    D3DRTYPE_VOLUMETEXTURE= 4,
    D3DRTYPE_CUBETEXTURE  = 5,
    D3DRTYPE_VERTEXBUFFER = 6,
    D3DRTYPE_INDEXBUFFER  = 7,
    D3DRTYPE_FORCE_DWORD  = 0x7fffffff
} D3DRESOURCETYPE;

typedef enum _D3DPOOL {
    D3DPOOL_DEFAULT = 0,
    D3DPOOL_MANAGED = 1,
    D3DPOOL_SYSTEMMEM = 2,
    D3DPOOL_SCRATCH = 3,
    D3DPOOL_FORCE_DWORD = 0x7fffffff
} D3DPOOL;

typedef enum _D3DPRIMITIVETYPE {
    D3DPT_POINTLIST     = 1,
    D3DPT_LINELIST      = 2,
    D3DPT_LINESTRIP     = 3,
    D3DPT_TRIANGLELIST  = 4,
    D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN   = 6,
    D3DPT_FORCE_DWORD   = 0x7fffffff
} D3DPRIMITIVETYPE;

typedef enum _D3DFORMAT {
    D3DFMT_UNKNOWN    = 0,
    D3DFMT_A8R8G8B8   = 21,
    D3DFMT_X8R8G8B8   = 22,
    D3DFMT_D16        = 80,
    D3DFMT_VERTEXDATA = 100,
    D3DFMT_INDEX16    = 101
} D3DFORMAT;

typedef enum _D3DBACKBUFFER_TYPE {
    D3DBACKBUFFER_TYPE_MONO  = 0,
    D3DBACKBUFFER_TYPE_LEFT  = 1,
    D3DBACKBUFFER_TYPE_RIGHT = 2,
    D3DBACKBUFFER_TYPE_FORCE_DWORD = 0x7fffffff
} D3DBACKBUFFER_TYPE;

typedef enum _D3DCULL {
    D3DCULL_NONE = 1,
    D3DCULL_CW   = 2,
    D3DCULL_CCW  = 3,
    D3DCULL_FORCE_DWORD = 0x7fffffff
} D3DCULL;

typedef enum _D3DRENDERSTATETYPE {
    D3DRS_ZENABLE          = 7,
    D3DRS_CULLMODE         = 22,
    D3DRS_ALPHABLENDENABLE = 27
} D3DRENDERSTATETYPE;

#define D3DRS_SOFTWAREVERTEXPROCESSING 153

typedef enum _D3DTRANSFORMSTATETYPE {
    D3DTS_VIEW       = 2,
    D3DTS_PROJECTION = 3,
    D3DTS_WORLD      = 256
} D3DTRANSFORMSTATETYPE;

typedef enum _D3DSWAPEFFECT {
    D3DSWAPEFFECT_DISCARD    = 1,
    D3DSWAPEFFECT_FLIP       = 2,
    D3DSWAPEFFECT_COPY       = 3,
    D3DSWAPEFFECT_COPY_VSYNC = 4,
    D3DSWAPEFFECT_FORCE_DWORD = 0x7fffffff
} D3DSWAPEFFECT;

/* Structures */
typedef struct _D3DVIEWPORT8 {
    DWORD X;
    DWORD Y;
    DWORD Width;
    DWORD Height;
    float MinZ;
    float MaxZ;
} D3DVIEWPORT8;

typedef struct _D3DDISPLAYMODE {
    UINT       Width;
    UINT       Height;
    UINT       RefreshRate;
    D3DFORMAT  Format;
} D3DDISPLAYMODE;

typedef struct _D3DPRESENT_PARAMETERS_ {
    UINT                BackBufferWidth;
    UINT                BackBufferHeight;
    D3DFORMAT           BackBufferFormat;
    UINT                BackBufferCount;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    D3DSWAPEFFECT       SwapEffect;
    HWND                hDeviceWindow;
    BOOL                Windowed;
    BOOL                EnableAutoDepthStencil;
    D3DFORMAT           AutoDepthStencilFormat;
    DWORD               Flags;
    UINT                FullScreen_RefreshRateInHz;
    UINT                FullScreen_PresentationInterval;
} D3DPRESENT_PARAMETERS;

typedef struct _D3DGAMMARAMP {
    WORD red[256];
    WORD green[256];
    WORD blue[256];
} D3DGAMMARAMP;

typedef struct _D3DRASTER_STATUS {
    BOOL InVBlank;
    UINT ScanLine;
} D3DRASTER_STATUS;

typedef struct _D3DDEVICE_CREATION_PARAMETERS {
    UINT        AdapterOrdinal;
    D3DDEVTYPE  DeviceType;
    HWND        hFocusWindow;
    DWORD       BehaviorFlags;
} D3DDEVICE_CREATION_PARAMETERS;

typedef struct _D3DVERTEXBUFFER_DESC {
    D3DFORMAT       Format;
    D3DRESOURCETYPE Type;
    DWORD           Usage;
    D3DPOOL         Pool;
    UINT            Size;
    DWORD           FVF;
} D3DVERTEXBUFFER_DESC;

typedef struct _D3DINDEXBUFFER_DESC {
    D3DFORMAT       Format;
    D3DRESOURCETYPE Type;
    DWORD           Usage;
    D3DPOOL         Pool;
    UINT            Size;
} D3DINDEXBUFFER_DESC;

typedef struct _D3DLOCKED_RECT {
    int Pitch;
    void *pBits;
} D3DLOCKED_RECT;

typedef struct _D3DSURFACE_DESC {
    D3DFORMAT       Format;
    D3DRESOURCETYPE Type;
    DWORD           Usage;
    D3DPOOL         Pool;
    UINT            Width;
    UINT            Height;
} D3DSURFACE_DESC;

typedef struct _D3DXVECTOR3 {
    float x, y, z;
} D3DXVECTOR3;

typedef struct _D3DXVECTOR4 {
    float x, y, z, w;
} D3DXVECTOR4;

typedef struct _D3DXMATRIX {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
} D3DXMATRIX, *LPD3DXMATRIX;

typedef struct _D3DADAPTER_IDENTIFIER8 {
    char  Driver[512];
    char  Description[512];
    unsigned long long DriverVersion;
    DWORD VendorId;
    DWORD DeviceId;
    DWORD SubSysId;
    DWORD Revision;
    GUID  DeviceIdentifier;
    DWORD WHQLLevel;
} D3DADAPTER_IDENTIFIER8;

typedef struct _D3DCAPS8 {
    D3DDEVTYPE DeviceType;
    UINT AdapterOrdinal;
    DWORD Caps;
    DWORD Caps2;
    DWORD Caps3;
    DWORD PresentationIntervals;
    DWORD CursorCaps;
    DWORD DevCaps;
    DWORD PrimitiveMiscCaps;
    DWORD RasterCaps;
    DWORD ZCmpCaps;
    DWORD SrcBlendCaps;
    DWORD DestBlendCaps;
    DWORD AlphaCmpCaps;
    DWORD ShadeCaps;
    DWORD TextureCaps;
    DWORD TextureFilterCaps;
    DWORD CubeTextureFilterCaps;
    DWORD VolumeTextureFilterCaps;
    DWORD TextureAddressCaps;
    DWORD VolumeTextureAddressCaps;
    DWORD LineCaps;
    DWORD MaxTextureWidth;
    DWORD MaxTextureHeight;
    DWORD MaxVolumeExtent;
    DWORD MaxTextureRepeat;
    DWORD MaxTextureAspectRatio;
    DWORD MaxAnisotropy;
    float MaxVertexW;
    float GuardBandLeft;
    float GuardBandTop;
    float GuardBandRight;
    float GuardBandBottom;
    float ExtentsAdjust;
    DWORD StencilCaps;
    DWORD FVFCaps;
    DWORD TextureOpCaps;
    DWORD MaxTextureBlendStages;
    DWORD MaxSimultaneousTextures;
    DWORD VertexProcessingCaps;
    DWORD MaxActiveLights;
    DWORD MaxUserClipPlanes;
    DWORD MaxVertexBlendMatrices;
    DWORD MaxVertexBlendMatrixIndex;
    float MaxPointSize;
    DWORD MaxPrimitiveCount;
    DWORD MaxVertexIndex;
    DWORD MaxStreams;
    DWORD MaxStreamStride;
    DWORD VertexShaderVersion;
    DWORD MaxVertexShaderConst;
    DWORD PixelShaderVersion;
    float MaxPixelShaderValue;
} D3DCAPS8;

/* Capability flags */
#define D3DCAPS_READ_SCANLINE           0x00020000L
#define D3DCAPS2_DYNAMICTEXTURES        0x20000000L
#define D3DCAPS2_CANRENDERWINDOWED      0x00080000L
#define D3DCAPS2_FULLSCREENGAMMA        0x00020000L
#define D3DDEVCAPS_EXECUTESYSTEMMEMORY  0x00000010L
#define D3DDEVCAPS_TLVERTEXSYSTEMMEMORY 0x00000040L
#define D3DDEVCAPS_TEXTURESYSTEMMEMORY  0x00000100L
#define D3DDEVCAPS_DRAWPRIMTLVERTEX     0x00000400L
#define D3DDEVCAPS_HWRASTERIZATION      0x00080000L
#define D3DDEVCAPS_PUREDEVICE           0x00400000L
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT  0x00010000L
#define D3DDEVCAPS_NPATCHES             0x01000000L

#define D3DPMISCCAPS_MASKZ              0x00000002L
#define D3DPMISCCAPS_CULLNONE           0x00000010L
#define D3DPMISCCAPS_CULLCW             0x00000020L
#define D3DPMISCCAPS_CULLCCW            0x00000040L
#define D3DPMISCCAPS_COLORWRITEENABLE   0x00000080L

#define D3DPRASTERCAPS_DITHER           0x00000001L
#define D3DPRASTERCAPS_ZTEST            0x00000010L
#define D3DPRASTERCAPS_FOGVERTEX        0x00000080L
#define D3DPRASTERCAPS_MIPMAPLODBIAS    0x00002000L
#define D3DPRASTERCAPS_ZBIAS            0x00004000L

#define D3DPCMPCAPS_NEVER               0x00000001L
#define D3DPCMPCAPS_LESS                0x00000002L
#define D3DPCMPCAPS_EQUAL               0x00000004L
#define D3DPCMPCAPS_LESSEQUAL           0x00000008L
#define D3DPCMPCAPS_GREATER             0x00000010L
#define D3DPCMPCAPS_NOTEQUAL            0x00000020L
#define D3DPCMPCAPS_GREATEREQUAL        0x00000040L
#define D3DPCMPCAPS_ALWAYS              0x00000080L

#define D3DPBLENDCAPS_ZERO              0x00000001L
#define D3DPBLENDCAPS_ONE               0x00000002L
#define D3DPBLENDCAPS_SRCCOLOR          0x00000004L
#define D3DPBLENDCAPS_INVSRCCOLOR       0x00000008L
#define D3DPBLENDCAPS_SRCALPHA          0x00000010L
#define D3DPBLENDCAPS_INVSRCALPHA       0x00000020L
#define D3DPBLENDCAPS_DESTALPHA         0x00000040L
#define D3DPBLENDCAPS_INVDESTALPHA      0x00000080L
#define D3DPBLENDCAPS_SRCALPHASAT       0x00000100L

#define D3DPSHADECAPS_COLORGOURAUDRGB   0x00000008L
#define D3DPSHADECAPS_ALPHAGOURAUDBLEND 0x00004000L
#define D3DPSHADECAPS_FOGGOURAUD        0x00080000L

#define D3DPTEXTURECAPS_PERSPECTIVE     0x00000001L
#define D3DPTEXTURECAPS_ALPHA           0x00000004L
#define D3DPTEXTURECAPS_MIPMAP          0x00000040L
#define D3DPTEXTURECAPS_CUBEMAP         0x00000800L

#define D3DPTFILTERCAPS_MINFPOINT       0x00000100L
#define D3DPTFILTERCAPS_MINFLINEAR      0x00000200L
#define D3DPTFILTERCAPS_MIPFPOINT       0x00010000L
#define D3DPTFILTERCAPS_MIPFLINEAR      0x00020000L
#define D3DPTFILTERCAPS_MAGFPOINT       0x01000000L
#define D3DPTFILTERCAPS_MAGFLINEAR      0x02000000L
#define D3DPTFILTERCAPS_MAGFANISOTROPIC 0x04000000L
#define D3DPTFILTERCAPS_MINFANISOTROPIC 0x00000400L

#define D3DPTADDRESSCAPS_WRAP           0x00000001L
#define D3DPTADDRESSCAPS_MIRROR         0x00000002L
#define D3DPTADDRESSCAPS_CLAMP          0x00000004L
#define D3DPTADDRESSCAPS_INDEPENDENTUV  0x00000010L

#define D3DSTENCILCAPS_KEEP             0x00000001L
#define D3DSTENCILCAPS_ZERO             0x00000002L
#define D3DSTENCILCAPS_REPLACE          0x00000004L
#define D3DSTENCILCAPS_INCRSAT          0x00000008L
#define D3DSTENCILCAPS_DECRSAT          0x00000010L
#define D3DSTENCILCAPS_INVERT           0x00000020L
#define D3DSTENCILCAPS_INCR             0x00000040L
#define D3DSTENCILCAPS_DECR             0x00000080L

#define D3DTEXOPCAPS_DISABLE            0x00000001L
#define D3DTEXOPCAPS_SELECTARG1         0x00000002L
#define D3DTEXOPCAPS_SELECTARG2         0x00000004L
#define D3DTEXOPCAPS_MODULATE           0x00000008L
#define D3DTEXOPCAPS_MODULATE2X         0x00000010L
#define D3DTEXOPCAPS_ADD                0x00000040L
#define D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR 0x00020000L
#define D3DTEXOPCAPS_BUMPENVMAP         0x00200000L
#define D3DTEXOPCAPS_BUMPENVMAPLUMINANCE 0x00400000L
#define D3DTEXOPCAPS_DOTPRODUCT3        0x00800000L

#define D3DFVFCAPS_TEXCOORDCOUNTMASK    0x0000ffffL

#define D3DVTXPCAPS_TEXGEN              0x00000001L
#define D3DVTXPCAPS_MATERIALSOURCE7     0x00000002L
#define D3DVTXPCAPS_DIRECTIONALLIGHTS   0x00000008L
#define D3DVTXPCAPS_LOCALVIEWER         0x00000020L

#define D3DPRESENT_INTERVAL_DEFAULT     0x00000000L
#define D3DPRESENT_INTERVAL_ONE         0x00000001L
#define D3DPRESENT_INTERVAL_IMMEDIATE   0x80000000L

#define D3DVS_VERSION(_Major,_Minor) (0xFFFE0000 | ((_Major)<<8) | (_Minor))

#define D3DFVF_XYZ    0x002
#define D3DFVF_NORMAL 0x010
#define D3DFVF_DIFFUSE 0x040
#define D3DFVF_TEX1   0x100
#define D3DFVF_TEXCOUNT_MASK 0xF00

#define D3DVSD_STREAM(_StreamNumber) ((DWORD)((0 << 29) | (_StreamNumber)))
#define D3DVSD_REG(_VertexRegister, _Type) \
    ((DWORD)((1 << 29) | ((_Type) << 16) | (_VertexRegister)))
#define D3DVSD_END() 0xFFFFFFFF

#define D3DVSDT_FLOAT2 0x01
#define D3DVSDT_FLOAT3 0x02
#define D3DVSDT_D3DCOLOR 0x04
#define D3DVSDE_POSITION 0
#define D3DVSDE_NORMAL   3
#define D3DVSDE_DIFFUSE  5
#define D3DVSDE_TEXCOORD0 7

#define D3DUSAGE_WRITEONLY    0x00000008L
#define D3DUSAGE_DYNAMIC      0x00000200L
#define D3DUSAGE_RENDERTARGET 0x00000001L
#define D3DUSAGE_DEPTHSTENCIL 0x00000002L

#ifdef __cplusplus
}
#endif

#endif /* D3D8_DEFS_H */
