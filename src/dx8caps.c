#include "dx8caps.h"
#include <string.h>

// Map OpenGL ES 1.1 capabilities to D3DCAPS8
void fill_d3d_caps(D3DCAPS8 *pCaps, D3DDEVTYPE DeviceType) {
    memset(pCaps, 0, sizeof(D3DCAPS8));
    pCaps->DeviceType = DeviceType;
    pCaps->AdapterOrdinal = D3DADAPTER_DEFAULT;
    pCaps->Caps = D3DCAPS_READ_SCANLINE;
    pCaps->Caps2 = D3DCAPS2_DYNAMICTEXTURES |
                   (DeviceType == D3DDEVTYPE_REF ? 0 : D3DCAPS2_CANRENDERWINDOWED);
    pCaps->PresentationIntervals =
        D3DPRESENT_INTERVAL_IMMEDIATE | D3DPRESENT_INTERVAL_ONE;
    pCaps->DevCaps = D3DDEVCAPS_EXECUTESYSTEMMEMORY |
                     D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
                     D3DDEVCAPS_TEXTURESYSTEMMEMORY | D3DDEVCAPS_DRAWPRIMTLVERTEX |
                     D3DDEVCAPS_HWRASTERIZATION |
                     (DeviceType == D3DDEVTYPE_REF ? 0 : D3DDEVCAPS_PUREDEVICE);
    pCaps->PrimitiveMiscCaps = D3DPMISCCAPS_MASKZ | D3DPMISCCAPS_CULLNONE |
                               D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLCCW |
                               D3DPMISCCAPS_COLORWRITEENABLE;
    pCaps->RasterCaps = D3DPRASTERCAPS_DITHER | D3DPRASTERCAPS_ZTEST |
                        D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_MIPMAPLODBIAS;
    pCaps->ZCmpCaps = pCaps->AlphaCmpCaps =
        D3DPCMPCAPS_NEVER | D3DPCMPCAPS_LESS | D3DPCMPCAPS_EQUAL |
        D3DPCMPCAPS_LESSEQUAL | D3DPCMPCAPS_GREATER |
        D3DPCMPCAPS_NOTEQUAL | D3DPCMPCAPS_GREATEREQUAL |
        D3DPCMPCAPS_ALWAYS;
    pCaps->SrcBlendCaps = pCaps->DestBlendCaps =
        D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE |
        D3DPBLENDCAPS_SRCCOLOR | D3DPBLENDCAPS_INVSRCCOLOR |
        D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA |
        D3DPBLENDCAPS_DESTALPHA | D3DPBLENDCAPS_INVDESTALPHA |
        D3DPBLENDCAPS_SRCALPHASAT;
    pCaps->ShadeCaps =
        D3DPSHADECAPS_COLORGOURAUDRGB | D3DPSHADECAPS_ALPHAGOURAUDBLEND |
        D3DPSHADECAPS_FOGGOURAUD;
    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    pCaps->MaxTextureWidth = pCaps->MaxTextureHeight = max_texture_size;
    pCaps->TextureCaps = D3DPTEXTURECAPS_PERSPECTIVE | D3DPTEXTURECAPS_ALPHA |
                         D3DPTEXTURECAPS_MIPMAP | D3DPTEXTURECAPS_CUBEMAP;
    pCaps->TextureFilterCaps = pCaps->CubeTextureFilterCaps =
        D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR |
        D3DPTFILTERCAPS_MIPFPOINT | D3DPTFILTERCAPS_MIPFLINEAR |
        D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR;
    pCaps->TextureAddressCaps = D3DPTADDRESSCAPS_WRAP | D3DPTADDRESSCAPS_MIRROR |
                                D3DPTADDRESSCAPS_CLAMP | D3DPTADDRESSCAPS_INDEPENDENTUV;
    pCaps->StencilCaps = D3DSTENCILCAPS_KEEP | D3DSTENCILCAPS_ZERO |
                         D3DSTENCILCAPS_REPLACE | D3DSTENCILCAPS_INCRSAT |
                         D3DSTENCILCAPS_DECRSAT | D3DSTENCILCAPS_INVERT |
                         D3DSTENCILCAPS_INCR | D3DSTENCILCAPS_DECR;
    pCaps->TextureOpCaps = D3DTEXOPCAPS_DISABLE | D3DTEXOPCAPS_SELECTARG1 |
                           D3DTEXOPCAPS_SELECTARG2 | D3DTEXOPCAPS_MODULATE |
                           D3DTEXOPCAPS_MODULATE2X | D3DTEXOPCAPS_ADD;
    pCaps->FVFCaps = D3DFVFCAPS_TEXCOORDCOUNTMASK & 0x8;
    pCaps->VertexProcessingCaps = D3DVTXPCAPS_TEXGEN |
                                  D3DVTXPCAPS_MATERIALSOURCE7 |
                                  D3DVTXPCAPS_DIRECTIONALLIGHTS |
                                  D3DVTXPCAPS_LOCALVIEWER;
    pCaps->MaxActiveLights = 8;
    pCaps->MaxUserClipPlanes = 0;
    pCaps->MaxVertexBlendMatrices = 4;
    pCaps->MaxStreams = 1;
    pCaps->MaxStreamStride = 256;
    pCaps->VertexShaderVersion = D3DVS_VERSION(1, 1);
    pCaps->MaxVertexShaderConst = 96;
    pCaps->PixelShaderVersion = 0;
    pCaps->MaxPixelShaderValue = 0.0f;
    pCaps->MaxTextureBlendStages = 2;
    pCaps->MaxSimultaneousTextures = 2;
    pCaps->MaxPrimitiveCount = 65535;
    pCaps->MaxVertexIndex = 65535;
    pCaps->MaxPointSize = 64.0f;
}

