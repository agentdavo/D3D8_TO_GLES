// src/d3d8_to_gles.c
#include "d3d8_to_gles.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdalign.h>
#include <EGL/eglext.h>

#ifdef D3D8_GLES_LOGGING
#include <stdio.h>
#include <stdarg.h>
void d3d8_gles_log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}
#endif

_Thread_local alignas(64) static D3DXMATRIX temp_matrix;

// Map OpenGL ES 1.1 capabilities to D3DCAPS8
void fill_d3d_caps(D3DCAPS8 *pCaps, D3DDEVTYPE DeviceType) {
    memset(pCaps, 0, sizeof(D3DCAPS8));
    pCaps->DeviceType = DeviceType;
    pCaps->AdapterOrdinal = D3DADAPTER_DEFAULT;
    pCaps->Caps = D3DCAPS_READ_SCANLINE;
    pCaps->Caps2 =
        D3DCAPS2_DYNAMICTEXTURES |
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
                                D3DPTADDRESSCAPS_CLAMP |
                                D3DPTADDRESSCAPS_INDEPENDENTUV;
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

// Forward declarations for device methods used before definition
static HRESULT D3DAPI d3d8_test_cooperative_level(IDirect3DDevice8 *This);
static UINT D3DAPI d3d8_get_available_texture_mem(IDirect3DDevice8 *This);
static HRESULT D3DAPI d3d8_resource_manager_discard_bytes(IDirect3DDevice8 *This, DWORD Bytes);
static HRESULT D3DAPI d3d8_get_direct3d(IDirect3DDevice8 *This, IDirect3D8 **ppD3D8);
static HRESULT D3DAPI device_get_device_caps(IDirect3DDevice8 *This, D3DCAPS8 *pCaps);
static HRESULT D3DAPI d3d8_get_display_mode(IDirect3DDevice8 *This, D3DDISPLAYMODE *pMode);
static HRESULT D3DAPI d3d8_get_creation_parameters(IDirect3DDevice8 *This, D3DDEVICE_CREATION_PARAMETERS *pParameters);
static HRESULT D3DAPI d3d8_set_cursor_properties(IDirect3DDevice8 *This, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8 *pCursorBitmap);
static void D3DAPI d3d8_set_cursor_position(IDirect3DDevice8 *This, UINT XScreenSpace, UINT YScreenSpace, DWORD Flags);
static BOOL D3DAPI d3d8_show_cursor(IDirect3DDevice8 *This, BOOL bShow);
static HRESULT D3DAPI d3d8_create_additional_swap_chain(IDirect3DDevice8 *This, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DSwapChain8 **pSwapChain);
static HRESULT D3DAPI d3d8_reset(IDirect3DDevice8 *This, D3DPRESENT_PARAMETERS *pPresentationParameters);
static HRESULT D3DAPI d3d8_present(IDirect3DDevice8 *This, CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion);
static HRESULT D3DAPI d3d8_get_back_buffer(IDirect3DDevice8 *This, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8 **ppBackBuffer);
static HRESULT D3DAPI d3d8_get_raster_status(IDirect3DDevice8 *This, D3DRASTER_STATUS *pRasterStatus);
static void D3DAPI d3d8_set_gamma_ramp(IDirect3DDevice8 *This, DWORD Flags, CONST D3DGAMMARAMP *pRamp);
static void D3DAPI d3d8_get_gamma_ramp(IDirect3DDevice8 *This, D3DGAMMARAMP *pRamp);
static HRESULT D3DAPI d3d8_create_vertex_buffer(IDirect3DDevice8 *This, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8 **ppVertexBuffer);
static HRESULT D3DAPI d3d8_create_index_buffer(IDirect3DDevice8 *This, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8 **ppIndexBuffer);
static HRESULT D3DAPI d3d8_set_render_state(IDirect3DDevice8 *This, D3DRENDERSTATETYPE state, DWORD value);
static HRESULT D3DAPI d3d8_begin_scene(IDirect3DDevice8 *This);
static HRESULT D3DAPI d3d8_end_scene(IDirect3DDevice8 *This);
static HRESULT D3DAPI d3d8_set_stream_source(IDirect3DDevice8 *This, UINT StreamNumber, IDirect3DVertexBuffer8 *pStreamData, UINT Stride);
static HRESULT D3DAPI d3d8_set_indices(IDirect3DDevice8 *This, IDirect3DIndexBuffer8 *pIndexData, UINT BaseVertexIndex);
static HRESULT D3DAPI d3d8_create_texture(IDirect3DDevice8 *This, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8 **ppTexture);
static HRESULT D3DAPI d3d8_set_texture(IDirect3DDevice8 *This, DWORD Stage, IDirect3DTexture8 *pTexture);
static HRESULT D3DAPI d3d8_set_texture_stage_state(IDirect3DDevice8 *This, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
static HRESULT D3DAPI d3d8_set_viewport(IDirect3DDevice8 *This, CONST D3DVIEWPORT8 *pViewport);
static HRESULT D3DAPI d3d8_set_transform(IDirect3DDevice8 *This, D3DTRANSFORMSTATETYPE State, CONST D3DXMATRIX *pMatrix);
static HRESULT D3DAPI d3d8_draw_indexed_primitive(IDirect3DDevice8 *This, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);

// Forward declarations for vertex buffer methods
static HRESULT D3DAPI d3d8_vb_get_device(IDirect3DVertexBuffer8 *This, IDirect3DDevice8 **ppDevice);
static HRESULT D3DAPI d3d8_vb_set_private_data(IDirect3DVertexBuffer8 *This, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags);
static HRESULT D3DAPI d3d8_vb_get_private_data(IDirect3DVertexBuffer8 *This, REFGUID refguid, void *pData, DWORD *pSizeOfData);
static HRESULT D3DAPI d3d8_vb_free_private_data(IDirect3DVertexBuffer8 *This, REFGUID refguid);
static DWORD D3DAPI d3d8_vb_set_priority(IDirect3DVertexBuffer8 *This, DWORD PriorityNew);
static DWORD D3DAPI d3d8_vb_get_priority(IDirect3DVertexBuffer8 *This);
static void D3DAPI d3d8_vb_pre_load(IDirect3DVertexBuffer8 *This);
static D3DRESOURCETYPE D3DAPI d3d8_vb_get_type(IDirect3DVertexBuffer8 *This);
static HRESULT D3DAPI d3d8_vb_lock(IDirect3DVertexBuffer8 *This, UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags);
static HRESULT D3DAPI d3d8_vb_unlock(IDirect3DVertexBuffer8 *This);
static HRESULT D3DAPI d3d8_vb_get_desc(IDirect3DVertexBuffer8 *This, D3DVERTEXBUFFER_DESC *pDesc);

// IUnknown-style wrappers for COM objects
static HRESULT D3DAPI d3d8_device_query_interface(IDirect3DDevice8 *This, REFIID riid, void **ppv);
static ULONG D3DAPI d3d8_device_add_ref(IDirect3DDevice8 *This);
static ULONG D3DAPI d3d8_device_release(IDirect3DDevice8 *This);
static HRESULT D3DAPI vb_query_interface(IDirect3DVertexBuffer8 *This, REFIID riid, void **ppv);
static ULONG D3DAPI vb_add_ref(IDirect3DVertexBuffer8 *This);
static ULONG D3DAPI vb_release(IDirect3DVertexBuffer8 *This);
static HRESULT D3DAPI ib_query_interface(IDirect3DIndexBuffer8 *This, REFIID riid, void **ppv);
static ULONG D3DAPI ib_add_ref(IDirect3DIndexBuffer8 *This);
static ULONG D3DAPI ib_release(IDirect3DIndexBuffer8 *This);

// Forward declarations for index buffer methods
static HRESULT D3DAPI d3d8_ib_get_device(IDirect3DIndexBuffer8 *This, IDirect3DDevice8 **ppDevice);
static HRESULT D3DAPI d3d8_ib_set_private_data(IDirect3DIndexBuffer8 *This, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags);
static HRESULT D3DAPI d3d8_ib_get_private_data(IDirect3DIndexBuffer8 *This, REFGUID refguid, void *pData, DWORD *pSizeOfData);
static HRESULT D3DAPI d3d8_ib_free_private_data(IDirect3DIndexBuffer8 *This, REFGUID refguid);
static DWORD D3DAPI d3d8_ib_set_priority(IDirect3DIndexBuffer8 *This, DWORD PriorityNew);
static DWORD D3DAPI d3d8_ib_get_priority(IDirect3DIndexBuffer8 *This);
static void D3DAPI d3d8_ib_pre_load(IDirect3DIndexBuffer8 *This);
static D3DRESOURCETYPE D3DAPI d3d8_ib_get_type(IDirect3DIndexBuffer8 *This);
static HRESULT D3DAPI d3d8_ib_lock(IDirect3DIndexBuffer8 *This, UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags);
static HRESULT D3DAPI d3d8_ib_unlock(IDirect3DIndexBuffer8 *This);
static HRESULT D3DAPI d3d8_ib_get_desc(IDirect3DIndexBuffer8 *This, D3DINDEXBUFFER_DESC *pDesc);

// Forward declarations for texture methods
static HRESULT D3DAPI tex_query_interface(IDirect3DTexture8 *This, REFIID riid, void **ppv);
static ULONG D3DAPI tex_add_ref(IDirect3DTexture8 *This);
static ULONG D3DAPI tex_release(IDirect3DTexture8 *This);
static HRESULT D3DAPI tex_lock_rect(IDirect3DTexture8 *This, UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags);
static HRESULT D3DAPI tex_unlock_rect(IDirect3DTexture8 *This, UINT Level);
static HRESULT D3DAPI tex_get_level_desc(IDirect3DTexture8 *This, UINT Level, D3DSURFACE_DESC *pDesc);

// Forward declarations for ID3DXBuffer helper methods
static HRESULT D3DAPI d3dx_buffer_query_interface(ID3DXBuffer *This, REFIID iid, void **ppv);
static ULONG D3DAPI d3dx_buffer_add_ref(ID3DXBuffer *This);
static ULONG D3DAPI d3dx_buffer_release(ID3DXBuffer *This);
static LPVOID d3dx_buffer_get_buffer_pointer(ID3DXBuffer *This);
static DWORD d3dx_buffer_get_buffer_size(ID3DXBuffer *This);

// Forward declarations for basic D3DX helpers
UINT WINAPI D3DXGetFVFVertexSize(DWORD FVF);
HRESULT WINAPI D3DXDeclaratorFromFVF(DWORD FVF, DWORD Declaration[MAX_FVF_DECL_SIZE]);

// Last created device's display mode for adapter queries
static D3DDISPLAYMODE g_current_display_mode;

static float dword_to_float(DWORD v) {
    union {
        DWORD d;
        float f;
    } u;
    u.d = v;
    return u.f;
}

static GLenum blend_to_gl(D3DBLEND blend) {
    switch (blend) {
        case D3DBLEND_ZERO: return GL_ZERO;
        case D3DBLEND_ONE: return GL_ONE;
        case D3DBLEND_SRCCOLOR: return GL_SRC_COLOR;
        case D3DBLEND_INVSRCCOLOR: return GL_ONE_MINUS_SRC_COLOR;
        case D3DBLEND_SRCALPHA: return GL_SRC_ALPHA;
        case D3DBLEND_INVSRCALPHA: return GL_ONE_MINUS_SRC_ALPHA;
        case D3DBLEND_DESTALPHA: return GL_DST_ALPHA;
        case D3DBLEND_INVDESTALPHA: return GL_ONE_MINUS_DST_ALPHA;
        case D3DBLEND_DESTCOLOR: return GL_DST_COLOR;
        case D3DBLEND_INVDESTCOLOR: return GL_ONE_MINUS_DST_COLOR;
        case D3DBLEND_SRCALPHASAT: return GL_SRC_ALPHA_SATURATE;
        default: return GL_ONE;
    }
}

static GLenum cmp_to_gl(D3DCMPFUNC func) {
    switch (func) {
        case D3DCMP_NEVER: return GL_NEVER;
        case D3DCMP_LESS: return GL_LESS;
        case D3DCMP_EQUAL: return GL_EQUAL;
        case D3DCMP_LESSEQUAL: return GL_LEQUAL;
        case D3DCMP_GREATER: return GL_GREATER;
        case D3DCMP_NOTEQUAL: return GL_NOTEQUAL;
        case D3DCMP_GREATEREQUAL: return GL_GEQUAL;
        case D3DCMP_ALWAYS: return GL_ALWAYS;
        default: return GL_ALWAYS;
    }
}

static GLenum stencil_op_to_gl(D3DSTENCILOP op) {
    switch (op) {
        case D3DSTENCILOP_KEEP: return GL_KEEP;
        case D3DSTENCILOP_ZERO: return GL_ZERO;
        case D3DSTENCILOP_REPLACE: return GL_REPLACE;
        case D3DSTENCILOP_INCRSAT: return GL_INCR;
        case D3DSTENCILOP_DECRSAT: return GL_DECR;
        case D3DSTENCILOP_INVERT: return GL_INVERT;
        case D3DSTENCILOP_INCR: return GL_INCR;
        case D3DSTENCILOP_DECR: return GL_DECR;
        default: return GL_KEEP;
    }
}

static GLenum fog_mode_to_gl(D3DFOGMODE mode) {
    switch (mode) {
        case D3DFOG_EXP: return GL_EXP;
        case D3DFOG_EXP2: return GL_EXP2;
        case D3DFOG_LINEAR: return GL_LINEAR;
        default: return GL_EXP;
    }
}

// Helper: Map Direct3D render state to OpenGL ES
static void set_render_state(GLES_Device *gles, D3DRENDERSTATETYPE state, DWORD value) {
    switch (state) {
        case D3DRS_ZENABLE:
            gles->depth_test = value;
            if (value) glEnable(GL_DEPTH_TEST);
            else glDisable(GL_DEPTH_TEST);
            break;
        case D3DRS_ZWRITEENABLE:
            glDepthMask(value ? GL_TRUE : GL_FALSE);
            break;
        case D3DRS_ALPHATESTENABLE:
            if (value) glEnable(GL_ALPHA_TEST);
            else glDisable(GL_ALPHA_TEST);
            break;
        case D3DRS_SRCBLEND:
            gles->src_blend = blend_to_gl((D3DBLEND)value);
            glBlendFunc(gles->src_blend, gles->dest_blend);
            break;
        case D3DRS_DESTBLEND:
            gles->dest_blend = blend_to_gl((D3DBLEND)value);
            glBlendFunc(gles->src_blend, gles->dest_blend);
            break;
        case D3DRS_CULLMODE:
            gles->cull_face = (value != D3DCULL_NONE);
            if (gles->cull_face) {
                glEnable(GL_CULL_FACE);
                gles->cull_mode = (value == D3DCULL_CW) ? GL_BACK : GL_FRONT;
                glCullFace(gles->cull_mode);
            } else {
                glDisable(GL_CULL_FACE);
            }
            break;
        case D3DRS_ZFUNC:
            gles->depth_func = cmp_to_gl((D3DCMPFUNC)value);
            glDepthFunc(gles->depth_func);
            break;
        case D3DRS_ALPHAREF:
            gles->alpha_ref = dword_to_float(value);
            glAlphaFunc(gles->alpha_func, gles->alpha_ref);
            break;
        case D3DRS_ALPHAFUNC:
            gles->alpha_func = cmp_to_gl((D3DCMPFUNC)value);
            glAlphaFunc(gles->alpha_func, gles->alpha_ref);
            break;
        case D3DRS_DITHERENABLE:
            if (value) glEnable(GL_DITHER); else glDisable(GL_DITHER);
            break;
        case D3DRS_ALPHABLENDENABLE:
            gles->blend = value;
            if (value) glEnable(GL_BLEND);
            else glDisable(GL_BLEND);
            break;
        case D3DRS_FOGENABLE:
            if (value) glEnable(GL_FOG); else glDisable(GL_FOG);
            break;
        case D3DRS_FOGCOLOR: {
            GLfloat color[4] = {
                (value & 0xFF) / 255.0f,
                ((value >> 8) & 0xFF) / 255.0f,
                ((value >> 16) & 0xFF) / 255.0f,
                ((value >> 24) & 0xFF) / 255.0f,
            };
            glFogfv(GL_FOG_COLOR, color);
            break;
        }
        case D3DRS_FOGTABLEMODE:
            gles->fog_mode = fog_mode_to_gl((D3DFOGMODE)value);
            glFogf(GL_FOG_MODE, (GLfloat)gles->fog_mode);
            break;
        case D3DRS_FOGSTART:
            glFogf(GL_FOG_START, dword_to_float(value));
            break;
        case D3DRS_FOGEND:
            glFogf(GL_FOG_END, dword_to_float(value));
            break;
        case D3DRS_FOGDENSITY:
            glFogf(GL_FOG_DENSITY, dword_to_float(value));
            break;
        case D3DRS_STENCILENABLE:
            gles->stencil_test = value;
            if (value) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
            break;
        case D3DRS_STENCILFUNC:
            gles->stencil_func = cmp_to_gl((D3DCMPFUNC)value);
            glStencilFunc(gles->stencil_func, gles->stencil_ref, gles->stencil_mask);
            break;
        case D3DRS_STENCILREF:
            gles->stencil_ref = (GLint)value;
            glStencilFunc(gles->stencil_func, gles->stencil_ref, gles->stencil_mask);
            break;
        case D3DRS_STENCILMASK:
            gles->stencil_mask = value;
            glStencilFunc(gles->stencil_func, gles->stencil_ref, gles->stencil_mask);
            break;
        case D3DRS_STENCILWRITEMASK:
            glStencilMask(value);
            break;
        case D3DRS_STENCILFAIL:
            gles->stencil_fail = stencil_op_to_gl((D3DSTENCILOP)value);
            glStencilOp(gles->stencil_fail, gles->stencil_zfail, gles->stencil_pass);
            break;
        case D3DRS_STENCILZFAIL:
            gles->stencil_zfail = stencil_op_to_gl((D3DSTENCILOP)value);
            glStencilOp(gles->stencil_fail, gles->stencil_zfail, gles->stencil_pass);
            break;
        case D3DRS_STENCILPASS:
            gles->stencil_pass = stencil_op_to_gl((D3DSTENCILOP)value);
            glStencilOp(gles->stencil_fail, gles->stencil_zfail, gles->stencil_pass);
            break;
        case D3DRS_COLORWRITEENABLE: {
            GLboolean r = (value & D3DCOLORWRITEENABLE_RED) ? GL_TRUE : GL_FALSE;
            GLboolean g = (value & D3DCOLORWRITEENABLE_GREEN) ? GL_TRUE : GL_FALSE;
            GLboolean b = (value & D3DCOLORWRITEENABLE_BLUE) ? GL_TRUE : GL_FALSE;
            GLboolean a = (value & D3DCOLORWRITEENABLE_ALPHA) ? GL_TRUE : GL_FALSE;
            glColorMask(r, g, b, a);
            break;
        }
        case D3DRS_LIGHTING:
            if (value) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
            break;
        case D3DRS_AMBIENT:
            gles->ambient[0] = (value & 0xFF) / 255.0f;
            gles->ambient[1] = ((value >> 8) & 0xFF) / 255.0f;
            gles->ambient[2] = ((value >> 16) & 0xFF) / 255.0f;
            gles->ambient[3] = ((value >> 24) & 0xFF) / 255.0f;
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, gles->ambient);
            break;
        default:
            d3d8_gles_log("Unsupported render state: %d\n", state);
    }
}

// Helper: Map D3DPRESENT_PARAMETERS to EGL config
static EGLConfig choose_egl_config(EGLDisplay display,
                                   D3DPRESENT_PARAMETERS *params,
                                   BOOL want_window) {
    EGLint config_attributes[] = {
        EGL_RED_SIZE, (params->BackBufferFormat == D3DFMT_A8R8G8B8 || params->BackBufferFormat == D3DFMT_X8R8G8B8) ? 8 : 5,
        EGL_GREEN_SIZE, (params->BackBufferFormat == D3DFMT_A8R8G8B8 || params->BackBufferFormat == D3DFMT_X8R8G8B8) ? 8 : 6,
        EGL_BLUE_SIZE, (params->BackBufferFormat == D3DFMT_A8R8G8B8 || params->BackBufferFormat == D3DFMT_X8R8G8B8) ? 8 : 5,
        EGL_ALPHA_SIZE, (params->BackBufferFormat == D3DFMT_A8R8G8B8) ? 8 : 0,
        EGL_DEPTH_SIZE, params->EnableAutoDepthStencil ? 16 : 0,
        EGL_STENCIL_SIZE, params->EnableAutoDepthStencil ? 8 : 0,
        EGL_SURFACE_TYPE, want_window ? EGL_WINDOW_BIT : EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
        EGL_SAMPLES, params->MultiSampleType >= D3DMULTISAMPLE_2_SAMPLES ? params->MultiSampleType : 0,
        EGL_NONE
    };

    EGLConfig config;
    EGLint num_configs;
    if (!eglChooseConfig(display, config_attributes, &config, 1, &num_configs) || num_configs == 0) {
        d3d8_gles_log("Failed to choose EGL config\n");
        return NULL;
    }
    return config;
}

// Helper: Convert Direct3D matrix to OpenGL (left-handed to right-handed, z=[0,1] to [-1,1])
static void d3d_to_gl_matrix(GLfloat *restrict gl_matrix,
                             const D3DXMATRIX *restrict d3d_matrix) {
    // Transpose and adjust for coordinate system differences
    gl_matrix[0] = d3d_matrix->_11; gl_matrix[4] = d3d_matrix->_21; gl_matrix[8] = d3d_matrix->_31; gl_matrix[12] = d3d_matrix->_41;
    gl_matrix[1] = d3d_matrix->_12; gl_matrix[5] = d3d_matrix->_22; gl_matrix[9] = d3d_matrix->_32; gl_matrix[13] = d3d_matrix->_42;
    gl_matrix[2] = -d3d_matrix->_13; gl_matrix[6] = -d3d_matrix->_23; gl_matrix[10] = -d3d_matrix->_33; gl_matrix[14] = -d3d_matrix->_43;
    gl_matrix[3] = d3d_matrix->_14; gl_matrix[7] = d3d_matrix->_24; gl_matrix[11] = d3d_matrix->_34; gl_matrix[15] = d3d_matrix->_44;
}

// Helper: Setup vertex attributes based on FVF
static void setup_vertex_attributes(GLES_Device *gles, DWORD fvf, BYTE *data,
                                    UINT stride) {
    GLint offset = 0;

    if (fvf & D3DFVF_XYZRHW) {
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(4, GL_FLOAT, stride, data + offset);
        offset += 16;
    } else if (fvf & D3DFVF_XYZ) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, stride, data + offset);
        offset += 12;
    } else {
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    if (fvf & D3DFVF_NORMAL) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, stride, data + offset);
        offset += 12;
    } else {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    if (fvf & D3DFVF_DIFFUSE) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_UNSIGNED_BYTE, stride, data + offset);
        offset += 4;
    } else {
        glDisableClientState(GL_COLOR_ARRAY);
    }

    if (fvf & D3DFVF_SPECULAR) {
        /* Skip specular color for now; GL ES 1.1 lacks secondary color arrays */
        offset += 4;
    }

    int tex_count = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    int limit = tex_count > 2 ? 2 : tex_count;
    for (int i = 0; i < limit; i++) {
        int unit = i;
        if (gles->texcoord_index0 < limit) {
            if (i == gles->texcoord_index0) {
                unit = 0;
            } else if (i < gles->texcoord_index0) {
                unit = i + 1;
            }
        }
        glClientActiveTexture(GL_TEXTURE0 + unit);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, stride, data + offset);
        offset += 8;
    }
    for (int i = limit; i < 2; i++) {
        glClientActiveTexture(GL_TEXTURE0 + i);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    glClientActiveTexture(GL_TEXTURE0);
}

// Math functions
D3DXMATRIX* WINAPI D3DXMatrixIdentity(D3DXMATRIX *pOut) {
    memset(pOut, 0, sizeof(D3DXMATRIX));
    pOut->_11 = pOut->_22 = pOut->_33 = pOut->_44 = 1.0f;
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixMultiply(D3DXMATRIX *restrict pOut,
                                      CONST D3DXMATRIX *restrict pM1,
                                      CONST D3DXMATRIX *restrict pM2) {
    D3DXMATRIX *restrict result = &temp_matrix;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result->m[i][j] = pM1->m[i][0] * pM2->m[0][j] +
                              pM1->m[i][1] * pM2->m[1][j] +
                              pM1->m[i][2] * pM2->m[2][j] +
                              pM1->m[i][3] * pM2->m[3][j];
        }
    }
    *pOut = *result;
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixLookAtLH(D3DXMATRIX *pOut, CONST D3DXVECTOR3 *pEye, CONST D3DXVECTOR3 *pAt, CONST D3DXVECTOR3 *pUp) {
    D3DXVECTOR3 zaxis, xaxis, yaxis;
    D3DXVec3Subtract(&zaxis, pAt, pEye);
    D3DXVec3Normalize(&zaxis, &zaxis);
    D3DXVec3Cross(&xaxis, pUp, &zaxis);
    D3DXVec3Normalize(&xaxis, &xaxis);
    D3DXVec3Cross(&yaxis, &zaxis, &xaxis);

    D3DXMatrixIdentity(pOut);
    pOut->_11 = xaxis.x; pOut->_12 = yaxis.x; pOut->_13 = zaxis.x;
    pOut->_21 = xaxis.y; pOut->_22 = yaxis.y; pOut->_23 = zaxis.y;
    pOut->_31 = xaxis.z; pOut->_32 = yaxis.z; pOut->_33 = zaxis.z;
    pOut->_41 = -D3DXVec3Dot(&xaxis, pEye);
    pOut->_42 = -D3DXVec3Dot(&yaxis, pEye);
    pOut->_43 = -D3DXVec3Dot(&zaxis, pEye);
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovLH(D3DXMATRIX *pOut, FLOAT fovy, FLOAT Aspect, FLOAT zn, FLOAT zf) {
    float yscale = 1.0f / tanf(fovy / 2.0f);
    float xscale = yscale / Aspect;
    D3DXMatrixIdentity(pOut);
    pOut->_11 = xscale;
    pOut->_22 = yscale;
    pOut->_33 = zf / (zf - zn);
    pOut->_34 = 1.0f;
    pOut->_43 = -zn * zf / (zf - zn);
    pOut->_44 = 0.0f;
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixTranspose(D3DXMATRIX *pOut, CONST D3DXMATRIX *pM) {
    const D3DXMATRIX *src = pM;
    D3DXMATRIX temp;
    if (pOut == pM) {
        temp = *pM;
        src = &temp;
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            pOut->m[i][j] = src->m[j][i];
        }
    }
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixInverse(D3DXMATRIX *pOut, FLOAT *pDeterminant, CONST D3DXMATRIX *pM) {
    float m[16] = {
        pM->_11, pM->_12, pM->_13, pM->_14,
        pM->_21, pM->_22, pM->_23, pM->_24,
        pM->_31, pM->_32, pM->_33, pM->_34,
        pM->_41, pM->_42, pM->_43, pM->_44
    };
    float inv[16];

    inv[0] = m[5]  * m[10] * m[15] - m[5]  * m[11] * m[14] - m[9]  * m[6]  * m[15] +
             m[9]  * m[7]  * m[14] + m[13] * m[6]  * m[11] - m[13] * m[7]  * m[10];
    inv[4] = -m[4]  * m[10] * m[15] + m[4]  * m[11] * m[14] + m[8]  * m[6]  * m[15] -
             m[8]  * m[7]  * m[14] - m[12] * m[6]  * m[11] + m[12] * m[7]  * m[10];
    inv[8] = m[4]  * m[9]  * m[15] - m[4]  * m[11] * m[13] - m[8]  * m[5]  * m[15] +
             m[8]  * m[7]  * m[13] + m[12] * m[5]  * m[11] - m[12] * m[7]  * m[9];
    inv[12] = -m[4]  * m[9]  * m[14] + m[4]  * m[10] * m[13] + m[8]  * m[5]  * m[14] -
              m[8]  * m[6]  * m[13] - m[12] * m[5]  * m[10] + m[12] * m[6]  * m[9];
    inv[1] = -m[1]  * m[10] * m[15] + m[1]  * m[11] * m[14] + m[9]  * m[2]  * m[15] -
             m[9]  * m[3]  * m[14] - m[13] * m[2]  * m[11] + m[13] * m[3]  * m[10];
    inv[5] = m[0]  * m[10] * m[15] - m[0]  * m[11] * m[14] - m[8]  * m[2]  * m[15] +
             m[8]  * m[3]  * m[14] + m[12] * m[2]  * m[11] - m[12] * m[3]  * m[10];
    inv[9] = -m[0]  * m[9]  * m[15] + m[0]  * m[11] * m[13] + m[8]  * m[1]  * m[15] -
             m[8]  * m[3]  * m[13] - m[12] * m[1]  * m[11] + m[12] * m[3]  * m[9];
    inv[13] = m[0]  * m[9]  * m[14] - m[0]  * m[10] * m[13] - m[8]  * m[1]  * m[14] +
              m[8]  * m[2]  * m[13] + m[12] * m[1]  * m[10] - m[12] * m[2]  * m[9];
    inv[2] = m[1]  * m[6]  * m[15] - m[1]  * m[7]  * m[14] - m[5]  * m[2]  * m[15] +
             m[5]  * m[3]  * m[14] + m[13] * m[2]  * m[7]  - m[13] * m[3]  * m[6];
    inv[6] = -m[0]  * m[6]  * m[15] + m[0]  * m[7]  * m[14] + m[4]  * m[2]  * m[15] -
             m[4]  * m[3]  * m[14] - m[12] * m[2]  * m[7]  + m[12] * m[3]  * m[6];
    inv[10] = m[0]  * m[5]  * m[15] - m[0]  * m[7]  * m[13] - m[4]  * m[1]  * m[15] +
              m[4]  * m[3]  * m[13] + m[12] * m[1]  * m[7]  - m[12] * m[3]  * m[5];
    inv[14] = -m[0]  * m[5]  * m[14] + m[0]  * m[6]  * m[13] + m[4]  * m[1]  * m[14] -
              m[4]  * m[2]  * m[13] - m[12] * m[1]  * m[6]  + m[12] * m[2]  * m[5];
    inv[3] = -m[1]  * m[6]  * m[11] + m[1]  * m[7]  * m[10] + m[5]  * m[2]  * m[11] -
             m[5]  * m[3]  * m[10] - m[9]  * m[2]  * m[7]  + m[9]  * m[3]  * m[6];
    inv[7] = m[0]  * m[6]  * m[11] - m[0]  * m[7]  * m[10] - m[4]  * m[2]  * m[11] +
             m[4]  * m[3]  * m[10] + m[8]  * m[2]  * m[7]  - m[8]  * m[3]  * m[6];
    inv[11] = -m[0] * m[5]  * m[11] + m[0] * m[7]  * m[9]  + m[4] * m[1]  * m[11] -
              m[4] * m[3]  * m[9]  - m[8] * m[1]  * m[7]  + m[8] * m[3]  * m[5];
    inv[15] = m[0] * m[5]  * m[10] - m[0] * m[6]  * m[9]  - m[4] * m[1]  * m[10] +
              m[4] * m[2]  * m[9]  + m[8] * m[1]  * m[6]  - m[8] * m[2]  * m[5];

    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (pDeterminant) *pDeterminant = det;
    if (fabsf(det) < 1e-6f)
        return NULL;

    det = 1.0f / det;
    for (int i = 0; i < 16; i++)
        inv[i] *= det;

    pOut->_11 = inv[0];  pOut->_12 = inv[1];  pOut->_13 = inv[2];  pOut->_14 = inv[3];
    pOut->_21 = inv[4];  pOut->_22 = inv[5];  pOut->_23 = inv[6];  pOut->_24 = inv[7];
    pOut->_31 = inv[8];  pOut->_32 = inv[9];  pOut->_33 = inv[10]; pOut->_34 = inv[11];
    pOut->_41 = inv[12]; pOut->_42 = inv[13]; pOut->_43 = inv[14]; pOut->_44 = inv[15];

    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3Normalize(D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV) {
    float length = sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
    if (length == 0.0f) {
        *pOut = *pV;
        return pOut;
    }
    pOut->x = pV->x / length;
    pOut->y = pV->y / length;
    pOut->z = pV->z / length;
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformCoord(D3DXVECTOR3 *restrict pOut,
                                           CONST D3DXVECTOR3 *restrict pV,
                                           CONST D3DXMATRIX *restrict pM) {
    D3DXVECTOR4 temp = { pV->x, pV->y, pV->z, 1.0f };
    pOut->x = temp.x * pM->_11 + temp.y * pM->_21 + temp.z * pM->_31 + temp.w * pM->_41;
    pOut->y = temp.x * pM->_12 + temp.y * pM->_22 + temp.z * pM->_32 + temp.w * pM->_42;
    pOut->z = temp.x * pM->_13 + temp.y * pM->_23 + temp.z * pM->_33 + temp.w * pM->_43;
    float w = temp.x * pM->_14 + temp.y * pM->_24 + temp.z * pM->_34 + temp.w * pM->_44;
    if (w != 0.0f) {
        pOut->x /= w;
        pOut->y /= w;
        pOut->z /= w;
    }
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3Subtract(D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2) {
    pOut->x = pV1->x - pV2->x;
    pOut->y = pV1->y - pV2->y;
    pOut->z = pV1->z - pV2->z;
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3Cross(D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2) {
    pOut->x = pV1->y * pV2->z - pV1->z * pV2->y;
    pOut->y = pV1->z * pV2->x - pV1->x * pV2->z;
    pOut->z = pV1->x * pV2->y - pV1->y * pV2->x;
    return pOut;
}

FLOAT WINAPI D3DXVec3Dot(CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2) {
    return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}

FLOAT WINAPI D3DXVec4Dot(CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2) {
    return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z + pV1->w * pV2->w;
}

D3DXVECTOR4* WINAPI D3DXVec4Transform(D3DXVECTOR4 *restrict pOut,
                                      CONST D3DXVECTOR4 *restrict pV,
                                      CONST D3DXMATRIX *restrict pM) {
    pOut->x = pV->x * pM->_11 + pV->y * pM->_21 + pV->z * pM->_31 + pV->w * pM->_41;
    pOut->y = pV->x * pM->_12 + pV->y * pM->_22 + pV->z * pM->_32 + pV->w * pM->_42;
    pOut->z = pV->x * pM->_13 + pV->y * pM->_23 + pV->z * pM->_33 + pV->w * pM->_43;
    pOut->w = pV->x * pM->_14 + pV->y * pM->_24 + pV->z * pM->_34 + pV->w * pM->_44;
    return pOut;
}

// ID3DXMatrixStack methods
static HRESULT D3DAPI d3dx_matrix_stack_query_interface(ID3DXMatrixStack *This, REFIID riid, LPVOID *ppvObj) {
    return D3DERR_INVALIDCALL;
}

static ULONG D3DAPI d3dx_matrix_stack_add_ref(ID3DXMatrixStack *This) { return 1; }
static ULONG D3DAPI d3dx_matrix_stack_release(ID3DXMatrixStack *This) {
    free(This->stack);
    free(This);
    return 0;
}

static HRESULT D3DAPI d3dx_matrix_stack_pop(ID3DXMatrixStack *This) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    This->top--;
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_matrix_stack_push(ID3DXMatrixStack *This) {
    if (This->top >= This->capacity) {
        DWORD new_capacity = This->capacity ? This->capacity * 2 : 16;
        D3DXMATRIX *new_stack = realloc(This->stack, new_capacity * sizeof(D3DXMATRIX));
        if (!new_stack) return D3DERR_OUTOFVIDEOMEMORY;
        This->stack = new_stack;
        This->capacity = new_capacity;
    }
    if (This->top > 0) This->stack[This->top] = This->stack[This->top - 1];
    else D3DXMatrixIdentity(&This->stack[This->top]);
    This->top++;
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_matrix_stack_load_identity(ID3DXMatrixStack *This) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    D3DXMatrixIdentity(&This->stack[This->top - 1]);
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_matrix_stack_load_matrix(ID3DXMatrixStack *This, CONST D3DXMATRIX *pM) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    This->stack[This->top - 1] = *pM;
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_matrix_stack_mult_matrix(ID3DXMatrixStack *This, CONST D3DXMATRIX *pM) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    D3DXMatrixMultiply(&This->stack[This->top - 1], &This->stack[This->top - 1], pM);
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_matrix_stack_mult_matrix_local(ID3DXMatrixStack *This, CONST D3DXMATRIX *pM) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    D3DXMATRIX temp;
    D3DXMatrixMultiply(&temp, pM, &This->stack[This->top - 1]);
    This->stack[This->top - 1] = temp;
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_matrix_stack_rotate_axis(ID3DXMatrixStack *This, CONST D3DXVECTOR3 *pV, FLOAT Angle) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    D3DXMATRIX rot;
    D3DXMatrixRotationAxis(&rot, pV, Angle);
    return d3dx_matrix_stack_mult_matrix(This, &rot);
}

static HRESULT D3DAPI d3dx_matrix_stack_rotate_axis_local(ID3DXMatrixStack *This, CONST D3DXVECTOR3 *pV, FLOAT Angle) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    D3DXMATRIX rot;
    D3DXMatrixRotationAxis(&rot, pV, Angle);
    return d3dx_matrix_stack_mult_matrix_local(This, &rot);
}

static HRESULT D3DAPI d3dx_matrix_stack_rotate_yaw_pitch_roll(ID3DXMatrixStack *This, FLOAT Yaw, FLOAT Pitch, FLOAT Roll) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    D3DXMATRIX rot;
    D3DXMatrixRotationYawPitchRoll(&rot, Yaw, Pitch, Roll);
    return d3dx_matrix_stack_mult_matrix(This, &rot);
}

static HRESULT D3DAPI d3dx_matrix_stack_rotate_yaw_pitch_roll_local(ID3DXMatrixStack *This, FLOAT Yaw, FLOAT Pitch, FLOAT Roll) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    D3DXMATRIX rot;
    D3DXMatrixRotationYawPitchRoll(&rot, Yaw, Pitch, Roll);
    return d3dx_matrix_stack_mult_matrix_local(This, &rot);
}

static HRESULT D3DAPI d3dx_matrix_stack_scale(ID3DXMatrixStack *This, FLOAT x, FLOAT y, FLOAT z) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    D3DXMATRIX scale;
    D3DXMatrixScaling(&scale, x, y, z);
    return d3dx_matrix_stack_mult_matrix(This, &scale);
}

static HRESULT D3DAPI d3dx_matrix_stack_scale_local(ID3DXMatrixStack *This, FLOAT x, FLOAT y, FLOAT z) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    D3DXMATRIX scale;
    D3DXMatrixScaling(&scale, x, y, z);
    return d3dx_matrix_stack_mult_matrix_local(This, &scale);
}

static HRESULT D3DAPI d3dx_matrix_stack_translate(ID3DXMatrixStack *This, FLOAT x, FLOAT y, FLOAT z) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    D3DXMATRIX trans;
    D3DXMatrixTranslation(&trans, x, y, z);
    return d3dx_matrix_stack_mult_matrix(This, &trans);
}

static HRESULT D3DAPI d3dx_matrix_stack_translate_local(ID3DXMatrixStack *This, FLOAT x, FLOAT y, FLOAT z) {
    if (This->top == 0) return D3DERR_INVALIDCALL;
    D3DXMATRIX trans;
    D3DXMatrixTranslation(&trans, x, y, z);
    return d3dx_matrix_stack_mult_matrix_local(This, &trans);
}

static D3DXMATRIX* d3dx_matrix_stack_get_top(ID3DXMatrixStack *This) {
    if (This->top == 0) return NULL;
    return &This->stack[This->top - 1];
}

// ID3DXMesh methods
static HRESULT D3DAPI d3dx_mesh_query_interface(void *This, REFIID iid, void **ppv) {
    return D3DERR_INVALIDCALL;
}

static ULONG D3DAPI d3dx_mesh_add_ref(void *This) { return 1; }
static ULONG D3DAPI d3dx_mesh_release(void *ptr) {
    ID3DXMesh *This = ptr;
    if (This->vb) This->vb->lpVtbl->Release(This->vb);
    if (This->ib) This->ib->lpVtbl->Release(This->ib);
    if (This->device) This->device->lpVtbl->Release(This->device);
    free(This->attrib_table);
    free(This->attrib_buffer);
    free(This);
    return 0;
}

static HRESULT D3DAPI d3dx_mesh_draw_subset(void *ptr, DWORD AttribId) {
    ID3DXMesh *This = ptr;
    if (AttribId >= This->attrib_table_size || !This->device) return D3DERR_INVALIDCALL;

    D3DXATTRIBUTERANGE *range = &This->attrib_table[AttribId];
    This->device->lpVtbl->SetStreamSource(This->device, 0, This->vb, D3DXGetFVFVertexSize(This->fvf));
    This->device->lpVtbl->SetIndices(This->device, This->ib, 0);
    This->device->gles->fvf = This->fvf;
    This->device->gles->attrib_id = AttribId;
    return This->device->lpVtbl->DrawIndexedPrimitive(This->device, D3DPT_TRIANGLELIST, range->VertexStart,
                                                     range->VertexCount, range->FaceStart * 3, range->FaceCount);
}

static DWORD D3DAPI d3dx_mesh_get_num_faces(void *ptr) {
    ID3DXMesh *This = ptr;
    return This->num_faces;
}

static DWORD D3DAPI d3dx_mesh_get_num_vertices(void *ptr) {
    ID3DXMesh *This = ptr;
    return This->num_vertices;
}

static DWORD D3DAPI d3dx_mesh_get_fvf(void *ptr) {
    ID3DXMesh *This = ptr;
    return This->fvf;
}

static HRESULT D3DAPI d3dx_mesh_get_declaration(void *ptr, DWORD Declaration[MAX_FVF_DECL_SIZE]) {
    ID3DXMesh *This = ptr;
    return D3DXDeclaratorFromFVF(This->fvf, Declaration);
}

static DWORD D3DAPI d3dx_mesh_get_options(void *ptr) {
    ID3DXMesh *This = ptr;
    return This->options;
}

static HRESULT D3DAPI d3dx_mesh_get_device(void *ptr, LPDIRECT3DDEVICE8 *ppDevice) {
    ID3DXMesh *This = ptr;
    *ppDevice = This->device;
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_mesh_clone_mesh_fvf(void *ptr, DWORD Options, DWORD FVF, LPDIRECT3DDEVICE8 pD3DDevice, LPD3DXMESH *ppCloneMesh) {
    ID3DXMesh *This = ptr;
    return D3DXERR_NOTAVAILABLE;
}

static HRESULT D3DAPI d3dx_mesh_clone_mesh(void *ptr, DWORD Options, CONST DWORD *pDeclaration, LPDIRECT3DDEVICE8 pD3DDevice, LPD3DXMESH *ppCloneMesh) {
    ID3DXMesh *This = ptr;
    return D3DXERR_NOTAVAILABLE;
}

static HRESULT D3DAPI d3dx_mesh_get_vertex_buffer(void *ptr, LPDIRECT3DVERTEXBUFFER8 *ppVB) {
    ID3DXMesh *This = ptr;
    *ppVB = This->vb;
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_mesh_get_index_buffer(void *ptr, LPDIRECT3DINDEXBUFFER8 *ppIB) {
    ID3DXMesh *This = ptr;
    *ppIB = This->ib;
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_mesh_lock_vertex_buffer(void *ptr, DWORD Flags, BYTE **ppData) {
    ID3DXMesh *This = ptr;
    return This->vb->lpVtbl->Lock(This->vb, 0, 0, ppData, Flags);
}

static HRESULT D3DAPI d3dx_mesh_unlock_vertex_buffer(void *ptr) {
    ID3DXMesh *This = ptr;
    return This->vb->lpVtbl->Unlock(This->vb);
}

static HRESULT D3DAPI d3dx_mesh_lock_index_buffer(void *ptr, DWORD Flags, BYTE **ppData) {
    ID3DXMesh *This = ptr;
    return This->ib->lpVtbl->Lock(This->ib, 0, 0, ppData, Flags);
}

static HRESULT D3DAPI d3dx_mesh_unlock_index_buffer(void *ptr) {
    ID3DXMesh *This = ptr;
    return This->ib->lpVtbl->Unlock(This->ib);
}

static HRESULT D3DAPI d3dx_mesh_get_attribute_table(void *ptr, D3DXATTRIBUTERANGE *pAttribTable, DWORD *pAttribTableSize) {
    ID3DXMesh *This = ptr;
    if (pAttribTableSize) *pAttribTableSize = This->attrib_table_size;
    if (pAttribTable && This->attrib_table) {
        memcpy(pAttribTable, This->attrib_table, This->attrib_table_size * sizeof(D3DXATTRIBUTERANGE));
    }
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_mesh_convert_point_reps_to_adjacency(void *ptr, CONST DWORD *pPRep, DWORD *pAdjacency) {
    ID3DXMesh *This = ptr;
    return D3DXERR_NOTAVAILABLE;
}

static HRESULT D3DAPI d3dx_mesh_convert_adjacency_to_point_reps(void *ptr, CONST DWORD *pAdjacency, DWORD *pPRep) {
    ID3DXMesh *This = ptr;
    return D3DXERR_NOTAVAILABLE;
}

static HRESULT D3DAPI d3dx_mesh_generate_adjacency(void *ptr, FLOAT Epsilon, DWORD *pAdjacency) {
    ID3DXMesh *This = ptr;
    return D3DXERR_NOTAVAILABLE;
}

static HRESULT D3DAPI d3dx_mesh_lock_attribute_buffer(void *ptr, DWORD Flags, DWORD **ppData) {
    ID3DXMesh *This = ptr;
    if (!This->attrib_buffer) return D3DERR_INVALIDCALL;
    *ppData = This->attrib_buffer;
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_mesh_unlock_attribute_buffer(void *ptr) {
    ID3DXMesh *This = ptr;
    return D3D_OK;
}

static HRESULT D3DAPI d3dx_mesh_optimize(void *ptr, DWORD Flags, CONST DWORD *pAdjacencyIn, DWORD *pAdjacencyOut, DWORD *pFaceRemap, LPD3DXBUFFER *ppVertexRemap, LPD3DXMESH *ppOptMesh) {
    ID3DXMesh *This = ptr;
    return D3DXERR_NOTAVAILABLE;
}

static HRESULT D3DAPI d3dx_mesh_optimize_inplace(void *ptr, DWORD Flags, CONST DWORD *pAdjacencyIn, DWORD *pAdjacencyOut, DWORD *pFaceRemap, LPD3DXBUFFER *ppVertexRemap) {
    ID3DXMesh *This = ptr;
    return D3DXERR_NOTAVAILABLE;
}

// IDirect3D8 methods
static HRESULT D3DAPI common_query_interface(void *This, REFIID riid, void **ppvObj) {
    (void)This; (void)riid; (void)ppvObj;
    return D3DERR_INVALIDCALL;
}

static ULONG D3DAPI common_add_ref(void *This) { (void)This; return 1; }
static ULONG D3DAPI common_release(void *This) { free(This); return 0; }

// IUnknown-style helper implementations
static HRESULT D3DAPI d3d8_device_query_interface(IDirect3DDevice8 *This, REFIID riid, void **ppv) {
    return common_query_interface(This, riid, ppv);
}
static ULONG D3DAPI d3d8_device_add_ref(IDirect3DDevice8 *This) { return common_add_ref(This); }
static ULONG D3DAPI d3d8_device_release(IDirect3DDevice8 *This) { return common_release(This); }
static HRESULT D3DAPI vb_query_interface(IDirect3DVertexBuffer8 *This, REFIID riid, void **ppv) {
    return common_query_interface(This, riid, ppv);
}
static ULONG D3DAPI vb_add_ref(IDirect3DVertexBuffer8 *This) { return common_add_ref(This); }
static ULONG D3DAPI vb_release(IDirect3DVertexBuffer8 *This) { return common_release(This); }
static HRESULT D3DAPI ib_query_interface(IDirect3DIndexBuffer8 *This, REFIID riid, void **ppv) {
    return common_query_interface(This, riid, ppv);
}
static ULONG D3DAPI ib_add_ref(IDirect3DIndexBuffer8 *This) { return common_add_ref(This); }
static ULONG D3DAPI ib_release(IDirect3DIndexBuffer8 *This) { return common_release(This); }
static HRESULT D3DAPI tex_query_interface(IDirect3DTexture8 *This, REFIID riid, void **ppv) { return common_query_interface(This, riid, ppv); }
static ULONG D3DAPI tex_add_ref(IDirect3DTexture8 *This) { return common_add_ref(This); }
static ULONG D3DAPI tex_release(IDirect3DTexture8 *This) {
    if (This && This->texture) {
        glDeleteTextures(1, &This->texture->tex_id);
        free(This->texture->temp_buffer);
        free(This->texture);
    }
    return common_release(This);
}
static HRESULT D3DAPI tex_lock_rect(IDirect3DTexture8 *This, UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) {
    (void)Flags;
    if (!pLockedRect || Level >= This->texture->levels) return D3DERR_INVALIDCALL;
    UINT w = This->texture->width >> Level;
    UINT h = This->texture->height >> Level;
    if (w == 0) w = 1;
    if (h == 0) h = 1;
    This->texture->temp_buffer = malloc(w * h * 4);
    if (!This->texture->temp_buffer) return D3DERR_OUTOFVIDEOMEMORY;
    pLockedRect->Pitch = w * 4;
    pLockedRect->pBits = This->texture->temp_buffer;
    return D3D_OK;
}
static HRESULT D3DAPI tex_unlock_rect(IDirect3DTexture8 *This, UINT Level) {
    if (!This->texture->temp_buffer || Level >= This->texture->levels) return D3DERR_INVALIDCALL;
    UINT w = This->texture->width >> Level;
    UINT h = This->texture->height >> Level;
    if (w == 0) w = 1;
    if (h == 0) h = 1;
    glBindTexture(GL_TEXTURE_2D, This->texture->tex_id);
    glTexSubImage2D(GL_TEXTURE_2D, Level, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, This->texture->temp_buffer);
    glBindTexture(GL_TEXTURE_2D, 0);
    free(This->texture->temp_buffer);
    This->texture->temp_buffer = NULL;
    return D3D_OK;
}
static HRESULT D3DAPI tex_get_level_desc(IDirect3DTexture8 *This, UINT Level, D3DSURFACE_DESC *pDesc) {
    if (!pDesc || Level >= This->texture->levels) return D3DERR_INVALIDCALL;
    pDesc->Format = This->texture->format;
    pDesc->Type = D3DRTYPE_TEXTURE;
    pDesc->Usage = 0;
    pDesc->Pool = D3DPOOL_MANAGED;
    pDesc->Width = This->texture->width >> Level ? This->texture->width >> Level : 1;
    pDesc->Height = This->texture->height >> Level ? This->texture->height >> Level : 1;
    return D3D_OK;
}

static const IDirect3DDevice8Vtbl device_vtbl = {
    .QueryInterface = d3d8_device_query_interface,
    .AddRef = d3d8_device_add_ref,
    .Release = d3d8_device_release,
    .TestCooperativeLevel = d3d8_test_cooperative_level,
    .GetAvailableTextureMem = d3d8_get_available_texture_mem,
    .ResourceManagerDiscardBytes = d3d8_resource_manager_discard_bytes,
    .GetDirect3D = d3d8_get_direct3d,
    .GetDeviceCaps = device_get_device_caps,
    .GetDisplayMode = d3d8_get_display_mode,
    .GetCreationParameters = d3d8_get_creation_parameters,
    .SetCursorProperties = d3d8_set_cursor_properties,
    .SetCursorPosition = d3d8_set_cursor_position,
    .ShowCursor = d3d8_show_cursor,
    .CreateAdditionalSwapChain = d3d8_create_additional_swap_chain,
    .Reset = d3d8_reset,
    .Present = d3d8_present,
    .GetBackBuffer = d3d8_get_back_buffer,
    .GetRasterStatus = d3d8_get_raster_status,
    .SetGammaRamp = d3d8_set_gamma_ramp,
    .GetGammaRamp = d3d8_get_gamma_ramp,
    .CreateVertexBuffer = d3d8_create_vertex_buffer,
    .CreateIndexBuffer = d3d8_create_index_buffer,
    .CreateTexture = d3d8_create_texture,
    .SetTexture = d3d8_set_texture,
    .SetTextureStageState = d3d8_set_texture_stage_state,
    .SetRenderState = d3d8_set_render_state,
    .BeginScene = d3d8_begin_scene,
    .EndScene = d3d8_end_scene,
    .SetStreamSource = d3d8_set_stream_source,
    .SetIndices = d3d8_set_indices,
    .SetViewport = d3d8_set_viewport,
    .SetTransform = d3d8_set_transform,
    .DrawIndexedPrimitive = d3d8_draw_indexed_primitive
};
static HRESULT D3DAPI d3d8_register_software_device(IDirect3D8 *This, void *pInitializeFunction) { return D3DERR_NOTAVAILABLE; }
static UINT D3DAPI d3d8_get_adapter_count(IDirect3D8 *This) { return 1; }
static HRESULT D3DAPI d3d8_get_adapter_identifier(IDirect3D8 *This,
                                                  UINT Adapter,
                                                  DWORD Flags,
                                                  D3DADAPTER_IDENTIFIER8 *pIdentifier) {
    if (Adapter != D3DADAPTER_DEFAULT || !pIdentifier) return D3DERR_INVALIDCALL;

    memset(pIdentifier, 0, sizeof(*pIdentifier));

    const char *renderer = (const char *)glGetString(GL_RENDERER);
    if (!renderer) renderer = "d3d8_to_gles";
    strncpy(pIdentifier->Driver, renderer, sizeof(pIdentifier->Driver) - 1);

    const char *vendor = (const char *)glGetString(GL_VENDOR);
    if (!vendor) vendor = "Unknown";
    strncpy(pIdentifier->Description, vendor, sizeof(pIdentifier->Description) - 1);

    return D3D_OK;
}
static UINT D3DAPI d3d8_get_adapter_mode_count(IDirect3D8 *This, UINT Adapter) { return 1; }
static HRESULT D3DAPI d3d8_enum_adapter_modes(IDirect3D8 *This,
                                              UINT Adapter,
                                              UINT Mode,
                                              D3DDISPLAYMODE *pMode) {
    if (Adapter != D3DADAPTER_DEFAULT || Mode > 0 || !pMode)
        return D3DERR_INVALIDCALL;

    *pMode = g_current_display_mode;
    return D3D_OK;
}
static HRESULT D3DAPI d3d8_get_adapter_display_mode(IDirect3D8 *This, UINT Adapter, D3DDISPLAYMODE *pMode) { return D3DERR_NOTAVAILABLE; }
static HRESULT D3DAPI d3d8_check_device_type(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed) { return D3D_OK; }
static HRESULT D3DAPI d3d8_check_device_format(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) { return D3D_OK; }
static HRESULT D3DAPI d3d8_check_device_multi_sample_type(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType) { return D3DERR_NOTAVAILABLE; }
static HRESULT D3DAPI d3d8_check_depth_stencil_match(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) { return D3D_OK; }
static HRESULT D3DAPI d3d8_get_device_caps(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8 *pCaps) {
    fill_d3d_caps(pCaps, DeviceType);
    return D3D_OK;
}
static HMONITOR D3DAPI d3d8_get_adapter_monitor(IDirect3D8 *This, UINT Adapter) { return NULL; }

static HRESULT D3DAPI d3d8_create_device(IDirect3D8 *This, UINT Adapter, D3DDEVTYPE DeviceType,
                                        HWND hFocusWindow, DWORD BehaviorFlags,
                                        D3DPRESENT_PARAMETERS *pPresentationParameters,
                                        IDirect3DDevice8 **ppReturnedDeviceInterface) {
    if (Adapter != D3DADAPTER_DEFAULT || !pPresentationParameters || DeviceType == D3DDEVTYPE_SW) {
        return D3DERR_INVALIDCALL;
    }

    GLES_Device *gles = calloc(1, sizeof(GLES_Device));
    if (!gles) return D3DERR_OUTOFVIDEOMEMORY;

    gles->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(gles->display, NULL, NULL)) {
        gles->display =
            eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
                                  (void *)(intptr_t)EGL_DEFAULT_DISPLAY,
                                  NULL);
        if (!eglInitialize(gles->display, NULL, NULL)) {
            free(gles);
            return D3DERR_INVALIDCALL;
        }
    }

    BOOL want_window = hFocusWindow != NULL;
    EGLConfig config =
        choose_egl_config(gles->display, pPresentationParameters, want_window);
    if (!config) {
        eglTerminate(gles->display);
        free(gles);
        return D3DERR_INVALIDCALL;
    }

    if (hFocusWindow) {
        gles->surface = eglCreateWindowSurface(
            gles->display, config,
            (EGLNativeWindowType)(uintptr_t)hFocusWindow, NULL);
    } else {
        const EGLint pbuffer_attribs[] = {EGL_WIDTH,
                                          pPresentationParameters->BackBufferWidth,
                                          EGL_HEIGHT,
                                          pPresentationParameters->BackBufferHeight,
                                          EGL_NONE};
        gles->surface =
            eglCreatePbufferSurface(gles->display, config, pbuffer_attribs);
    }
    gles->context = eglCreateContext(gles->display, config, EGL_NO_CONTEXT, NULL);
    if (!gles->surface || !gles->context || !eglMakeCurrent(gles->display, gles->surface, gles->surface, gles->context)) {
        if (gles->context) eglDestroyContext(gles->display, gles->context);
        if (gles->surface) eglDestroySurface(gles->display, gles->surface);
        eglTerminate(gles->display);
        free(gles);
        return D3DERR_INVALIDCALL;
    }

    gles->viewport.X = 0;
    gles->viewport.Y = 0;
    gles->viewport.Width = pPresentationParameters->BackBufferWidth;
    gles->viewport.Height = pPresentationParameters->BackBufferHeight;
    gles->viewport.MinZ = 0.0f;
    gles->viewport.MaxZ = 1.0f;
    glViewport(0, 0, gles->viewport.Width, gles->viewport.Height);
    D3DXMatrixIdentity(&gles->world_matrix);
    D3DXMatrixIdentity(&gles->view_matrix);
    D3DXMatrixIdentity(&gles->projection_matrix);
    gles->src_blend = GL_ONE;
    gles->dest_blend = GL_ZERO;
    gles->alpha_func = GL_ALWAYS;
    gles->alpha_ref = 0.0f;
    gles->depth_func = GL_LEQUAL;
    gles->fog_mode = GL_EXP;
    gles->stencil_test = GL_FALSE;
    gles->stencil_func = GL_ALWAYS;
    gles->stencil_ref = 0;
    gles->stencil_mask = 0xFFFFFFFFu;
    gles->stencil_fail = GL_KEEP;
    gles->stencil_zfail = GL_KEEP;
    gles->stencil_pass = GL_KEEP;
    gles->texcoord_index0 = 0;
    gles->present_params = *pPresentationParameters;
    gles->display_mode.Width = pPresentationParameters->BackBufferWidth;
    gles->display_mode.Height = pPresentationParameters->BackBufferHeight;
    gles->display_mode.Format = pPresentationParameters->BackBufferFormat;
    gles->display_mode.RefreshRate = pPresentationParameters->FullScreen_RefreshRateInHz;

    g_current_display_mode = gles->display_mode;

    IDirect3DDevice8 *device = calloc(1, sizeof(IDirect3DDevice8) + sizeof(IDirect3DDevice8Vtbl));
    if (!device) {
        eglDestroyContext(gles->display, gles->context);
        eglDestroySurface(gles->display, gles->surface);
        eglTerminate(gles->display);
        free(gles);
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    device->lpVtbl = &device_vtbl;
    device->gles = gles;
    device->d3d8 = This;

    *ppReturnedDeviceInterface = device;
    return D3D_OK;
}

// IDirect3DDevice8 methods
static HRESULT D3DAPI d3d8_test_cooperative_level(IDirect3DDevice8 *This) { return D3D_OK; }
static UINT D3DAPI d3d8_get_available_texture_mem(IDirect3DDevice8 *This) { return 1024 * 1024 * 256; }
static HRESULT D3DAPI d3d8_resource_manager_discard_bytes(IDirect3DDevice8 *This, DWORD Bytes) { return D3D_OK; }
static HRESULT D3DAPI d3d8_get_direct3d(IDirect3DDevice8 *This, IDirect3D8 **ppD3D8) {
    *ppD3D8 = This->d3d8;
    return D3D_OK;
}
static HRESULT D3DAPI device_get_device_caps(IDirect3DDevice8 *This, D3DCAPS8 *pCaps) {
    fill_d3d_caps(pCaps, D3DDEVTYPE_HAL);
    return D3D_OK;
}
static HRESULT D3DAPI d3d8_get_display_mode(IDirect3DDevice8 *This, D3DDISPLAYMODE *pMode) {
    if (!pMode) return D3DERR_INVALIDCALL;
    *pMode = This->gles->display_mode;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_get_creation_parameters(IDirect3DDevice8 *This, D3DDEVICE_CREATION_PARAMETERS *pParameters) {
    if (!pParameters) return D3DERR_INVALIDCALL;
    pParameters->AdapterOrdinal = D3DADAPTER_DEFAULT;
    pParameters->DeviceType = D3DDEVTYPE_HAL;
    pParameters->hFocusWindow = This->gles->present_params.hDeviceWindow;
    pParameters->BehaviorFlags = 0;
    return D3D_OK;
}
static HRESULT D3DAPI d3d8_set_cursor_properties(IDirect3DDevice8 *This, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8 *pCursorBitmap) { return D3DERR_NOTAVAILABLE; }
static void D3DAPI d3d8_set_cursor_position(IDirect3DDevice8 *This, UINT XScreenSpace, UINT YScreenSpace, DWORD Flags) {}
static BOOL D3DAPI d3d8_show_cursor(IDirect3DDevice8 *This, BOOL bShow) { return FALSE; }
static HRESULT D3DAPI d3d8_create_additional_swap_chain(IDirect3DDevice8 *This, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DSwapChain8 **pSwapChain) { return D3DERR_NOTAVAILABLE; }
static HRESULT D3DAPI d3d8_reset(IDirect3DDevice8 *This, D3DPRESENT_PARAMETERS *pPresentationParameters) { return D3DERR_NOTAVAILABLE; }
static HRESULT D3DAPI d3d8_present(IDirect3DDevice8 *This, CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion) {
    eglSwapBuffers(This->gles->display, This->gles->surface);
    return D3D_OK;
}
static HRESULT D3DAPI d3d8_get_back_buffer(IDirect3DDevice8 *This, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8 **ppBackBuffer) { return D3DERR_NOTAVAILABLE; }
static HRESULT D3DAPI d3d8_get_raster_status(IDirect3DDevice8 *This, D3DRASTER_STATUS *pRasterStatus) { return D3DERR_NOTAVAILABLE; }
static void D3DAPI d3d8_set_gamma_ramp(IDirect3DDevice8 *This, DWORD Flags, CONST D3DGAMMARAMP *pRamp) {}
static void D3DAPI d3d8_get_gamma_ramp(IDirect3DDevice8 *This, D3DGAMMARAMP *pRamp) {}
static HRESULT D3DAPI d3d8_begin_scene(IDirect3DDevice8 *This) {
    d3d8_gles_log("BeginScene\n");
    return D3D_OK;
}
static HRESULT D3DAPI d3d8_end_scene(IDirect3DDevice8 *This) {
    d3d8_gles_log("EndScene\n");
    return D3D_OK;
}
static HRESULT D3DAPI d3d8_set_viewport(IDirect3DDevice8 *This, CONST D3DVIEWPORT8 *pViewport) {
    This->gles->viewport = *pViewport;
    glViewport(pViewport->X, pViewport->Y, pViewport->Width, pViewport->Height);
#ifdef GL_VERSION_ES_CM_1_0
    glDepthRangef(pViewport->MinZ, pViewport->MaxZ);
#else
    glDepthRange(pViewport->MinZ, pViewport->MaxZ);
#endif
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_set_transform(IDirect3DDevice8 *This, D3DTRANSFORMSTATETYPE State, CONST D3DXMATRIX *pMatrix) {
    switch (State) {
        case D3DTS_WORLD:
            This->gles->world_matrix = *pMatrix;
            break;
        case D3DTS_VIEW:
            This->gles->view_matrix = *pMatrix;
            break;
        case D3DTS_PROJECTION:
            This->gles->projection_matrix = *pMatrix;
            break;
        default:
            return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_draw_indexed_primitive(IDirect3DDevice8 *This, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount) {
    GLenum mode;
    GLsizei count;
    switch (PrimitiveType) {
        case D3DPT_TRIANGLELIST:
            mode = GL_TRIANGLES;
            count = PrimitiveCount * 3;
            break;
        case D3DPT_TRIANGLESTRIP:
            mode = GL_TRIANGLE_STRIP;
            count = PrimitiveCount + 2;
            break;
        case D3DPT_POINTLIST:
            mode = GL_POINTS;
            count = PrimitiveCount;
            break;
        default:
            return D3DERR_NOTAVAILABLE;
    }

    // Apply transformations
    D3DXMATRIX wvp;
    D3DXMatrixMultiply(&wvp, &This->gles->world_matrix, &This->gles->view_matrix);
    D3DXMatrixMultiply(&wvp, &wvp, &This->gles->projection_matrix);
    GLfloat gl_matrix[16];
    d3d_to_gl_matrix(gl_matrix, &wvp);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(gl_matrix);

    // Setup vertex attributes
    BYTE *vb_data;
    UINT stride = D3DXGetFVFVertexSize(This->gles->fvf);
    if (This->gles->current_vbo) {
        glBindBuffer(GL_ARRAY_BUFFER, This->gles->current_vbo);
        setup_vertex_attributes(This->gles, This->gles->fvf, 0, stride);
    } else {
        return D3DERR_INVALIDCALL;
    }

    // Draw
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, This->gles->current_ibo);
    glDrawElements(mode, count, GL_UNSIGNED_SHORT,
                   (void *)(StartIndex * sizeof(WORD)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return D3D_OK;
}

// Vertex/Index Buffer methods
static HRESULT D3DAPI d3d8_create_vertex_buffer(IDirect3DDevice8 *This, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8 **ppVertexBuffer) {
    GLES_Buffer *buffer = calloc(1, sizeof(GLES_Buffer));
    if (!buffer) return D3DERR_OUTOFVIDEOMEMORY;

    buffer->length = Length;
    buffer->usage = Usage;
    buffer->fvf = FVF;
    buffer->pool = Pool;

    glGenBuffers(1, &buffer->vbo_id);
    glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo_id);
    GLenum gl_usage = (Usage & D3DUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    glBufferData(GL_ARRAY_BUFFER, Length, NULL, gl_usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    IDirect3DVertexBuffer8 *vb = calloc(1, sizeof(IDirect3DVertexBuffer8) + sizeof(IDirect3DVertexBuffer8Vtbl));
    if (!vb) {
        glDeleteBuffers(1, &buffer->vbo_id);
        free(buffer);
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    static const IDirect3DVertexBuffer8Vtbl vb_vtbl = {
        .QueryInterface = vb_query_interface,
        .AddRef = vb_add_ref,
        .Release = vb_release,
        .GetDevice = d3d8_vb_get_device,
        .SetPrivateData = d3d8_vb_set_private_data,
        .GetPrivateData = d3d8_vb_get_private_data,
        .FreePrivateData = d3d8_vb_free_private_data,
        .SetPriority = d3d8_vb_set_priority,
        .GetPriority = d3d8_vb_get_priority,
        .PreLoad = d3d8_vb_pre_load,
        .GetType = d3d8_vb_get_type,
        .Lock = d3d8_vb_lock,
        .Unlock = d3d8_vb_unlock,
        .GetDesc = d3d8_vb_get_desc
    };
    vb->lpVtbl = &vb_vtbl;
    vb->buffer = buffer;
    vb->device = This;

    *ppVertexBuffer = vb;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_set_render_state(IDirect3DDevice8 *This, D3DRENDERSTATETYPE state, DWORD value) {
    set_render_state(This->gles, state, value);
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_create_index_buffer(IDirect3DDevice8 *This, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8 **ppIndexBuffer) {
    GLES_Buffer *buffer = calloc(1, sizeof(GLES_Buffer));
    if (!buffer) return D3DERR_OUTOFVIDEOMEMORY;

    buffer->length = Length;
    buffer->usage = Usage;
    buffer->format = Format;
    buffer->pool = Pool;

    glGenBuffers(1, &buffer->vbo_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->vbo_id);
    GLenum gl_usage = (Usage & D3DUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Length, NULL, gl_usage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    IDirect3DIndexBuffer8 *ib = calloc(1, sizeof(IDirect3DIndexBuffer8) + sizeof(IDirect3DIndexBuffer8Vtbl));
    if (!ib) {
        glDeleteBuffers(1, &buffer->vbo_id);
        free(buffer);
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    static const IDirect3DIndexBuffer8Vtbl ib_vtbl = {
        .QueryInterface = ib_query_interface,
        .AddRef = ib_add_ref,
        .Release = ib_release,
        .GetDevice = d3d8_ib_get_device,
        .SetPrivateData = d3d8_ib_set_private_data,
        .GetPrivateData = d3d8_ib_get_private_data,
        .FreePrivateData = d3d8_ib_free_private_data,
        .SetPriority = d3d8_ib_set_priority,
        .GetPriority = d3d8_ib_get_priority,
        .PreLoad = d3d8_ib_pre_load,
        .GetType = d3d8_ib_get_type,
        .Lock = d3d8_ib_lock,
        .Unlock = d3d8_ib_unlock,
        .GetDesc = d3d8_ib_get_desc
    };
    ib->lpVtbl = &ib_vtbl;
    ib->buffer = buffer;
    ib->device = This;

    *ppIndexBuffer = ib;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_set_stream_source(IDirect3DDevice8 *This, UINT StreamNumber, IDirect3DVertexBuffer8 *pStreamData, UINT Stride) {
    if (!pStreamData) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        This->gles->current_vbo = 0;
        return D3D_OK;
    }
    glBindBuffer(GL_ARRAY_BUFFER, pStreamData->buffer->vbo_id);
    This->gles->current_vbo = pStreamData->buffer->vbo_id;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_set_indices(IDirect3DDevice8 *This, IDirect3DIndexBuffer8 *pIndexData, UINT BaseVertexIndex) {
    if (!pIndexData) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        This->gles->current_ibo = 0;
        return D3D_OK;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIndexData->buffer->vbo_id);
    This->gles->current_ibo = pIndexData->buffer->vbo_id;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_vb_get_device(IDirect3DVertexBuffer8 *This, IDirect3DDevice8 **ppDevice) {
    *ppDevice = This->device;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_vb_set_private_data(IDirect3DVertexBuffer8 *This, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) { return D3DERR_NOTAVAILABLE; }
static HRESULT D3DAPI d3d8_vb_get_private_data(IDirect3DVertexBuffer8 *This, REFGUID refguid, void *pData, DWORD *pSizeOfData) { return D3DERR_NOTAVAILABLE; }
static HRESULT D3DAPI d3d8_vb_free_private_data(IDirect3DVertexBuffer8 *This, REFGUID refguid) { return D3DERR_NOTAVAILABLE; }
static DWORD D3DAPI d3d8_vb_set_priority(IDirect3DVertexBuffer8 *This, DWORD PriorityNew) { return 0; }
static DWORD D3DAPI d3d8_vb_get_priority(IDirect3DVertexBuffer8 *This) { return 0; }
static void D3DAPI d3d8_vb_pre_load(IDirect3DVertexBuffer8 *This) {}
static D3DRESOURCETYPE D3DAPI d3d8_vb_get_type(IDirect3DVertexBuffer8 *This) { return D3DRTYPE_VERTEXBUFFER; }

static HRESULT D3DAPI d3d8_vb_lock(IDirect3DVertexBuffer8 *This, UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags) {
    GLES_Buffer *buffer = This->buffer;
    if (SizeToLock == 0) SizeToLock = buffer->length - OffsetToLock;

    buffer->lock_offset = OffsetToLock;
    buffer->lock_size = SizeToLock;

    buffer->temp_buffer = malloc(buffer->length);
    if (!buffer->temp_buffer) return D3DERR_OUTOFVIDEOMEMORY;

    glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo_id);
    if (Flags & D3DLOCK_DISCARD) {
        GLenum usage = (buffer->usage & D3DUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
        glBufferData(GL_ARRAY_BUFFER, buffer->length, NULL, usage);
    }
    *ppbData = buffer->temp_buffer + OffsetToLock;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_vb_unlock(IDirect3DVertexBuffer8 *This) {
    GLES_Buffer *buffer = This->buffer;
    if (!buffer->temp_buffer) return D3DERR_INVALIDCALL;

    glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo_id);
    glBufferSubData(GL_ARRAY_BUFFER, buffer->lock_offset, buffer->lock_size,
                    buffer->temp_buffer + buffer->lock_offset);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    free(buffer->temp_buffer);
    buffer->temp_buffer = NULL;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_vb_get_desc(IDirect3DVertexBuffer8 *This, D3DVERTEXBUFFER_DESC *pDesc) {
    pDesc->Format = D3DFMT_VERTEXDATA;
    pDesc->Type = D3DRTYPE_VERTEXBUFFER;
    pDesc->Usage = This->buffer->usage;
    pDesc->Pool = This->buffer->pool;
    pDesc->Size = This->buffer->length;
    pDesc->FVF = This->buffer->fvf;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_ib_get_device(IDirect3DIndexBuffer8 *This, IDirect3DDevice8 **ppDevice) {
    *ppDevice = This->device;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_ib_set_private_data(IDirect3DIndexBuffer8 *This, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) { return D3DERR_NOTAVAILABLE; }
static HRESULT D3DAPI d3d8_ib_get_private_data(IDirect3DIndexBuffer8 *This, REFGUID refguid, void *pData, DWORD *pSizeOfData) { return D3DERR_NOTAVAILABLE; }
static HRESULT D3DAPI d3d8_ib_free_private_data(IDirect3DIndexBuffer8 *This, REFGUID refguid) { return D3DERR_NOTAVAILABLE; }
static DWORD D3DAPI d3d8_ib_set_priority(IDirect3DIndexBuffer8 *This, DWORD PriorityNew) { return 0; }
static DWORD D3DAPI d3d8_ib_get_priority(IDirect3DIndexBuffer8 *This) { return 0; }
static void D3DAPI d3d8_ib_pre_load(IDirect3DIndexBuffer8 *This) {}
static D3DRESOURCETYPE D3DAPI d3d8_ib_get_type(IDirect3DIndexBuffer8 *This) { return D3DRTYPE_INDEXBUFFER; }

static HRESULT D3DAPI d3d8_ib_lock(IDirect3DIndexBuffer8 *This, UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags) {
    GLES_Buffer *buffer = This->buffer;
    if (SizeToLock == 0) SizeToLock = buffer->length - OffsetToLock;

    buffer->lock_offset = OffsetToLock;
    buffer->lock_size = SizeToLock;

    buffer->temp_buffer = malloc(buffer->length);
    if (!buffer->temp_buffer) return D3DERR_OUTOFVIDEOMEMORY;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->vbo_id);
    if (Flags & D3DLOCK_DISCARD) {
        GLenum usage = (buffer->usage & D3DUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, buffer->length, NULL, usage);
    }
    *ppbData = buffer->temp_buffer + OffsetToLock;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_ib_unlock(IDirect3DIndexBuffer8 *This) {
    GLES_Buffer *buffer = This->buffer;
    if (!buffer->temp_buffer) return D3DERR_INVALIDCALL;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->vbo_id);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, buffer->lock_offset, buffer->lock_size,
                    buffer->temp_buffer + buffer->lock_offset);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    free(buffer->temp_buffer);
    buffer->temp_buffer = NULL;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_ib_get_desc(IDirect3DIndexBuffer8 *This, D3DINDEXBUFFER_DESC *pDesc) {
    pDesc->Format = This->buffer->format;
    pDesc->Type = D3DRTYPE_INDEXBUFFER;
    pDesc->Usage = This->buffer->usage;
    pDesc->Pool = This->buffer->pool;
    pDesc->Size = This->buffer->length;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_create_texture(IDirect3DDevice8 *This, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8 **ppTexture) {
    (void)Usage;
    (void)Pool;
    GLES_Texture *tex = calloc(1, sizeof(GLES_Texture));
    if (!tex) return D3DERR_OUTOFVIDEOMEMORY;

    tex->width = Width;
    tex->height = Height;
    tex->levels = Levels ? Levels : 1;
    tex->format = Format;

    glGenTextures(1, &tex->tex_id);
    glBindTexture(GL_TEXTURE_2D, tex->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    UINT w = Width, h = Height;
    for (UINT level = 0; level < tex->levels; level++) {
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        if (w > 1) w >>= 1;
        if (h > 1) h >>= 1;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    IDirect3DTexture8 *texture = calloc(1, sizeof(IDirect3DTexture8) + sizeof(IDirect3DTexture8Vtbl));
    if (!texture) {
        glDeleteTextures(1, &tex->tex_id);
        free(tex);
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    static const IDirect3DTexture8Vtbl tex_vtbl = {
        .QueryInterface = tex_query_interface,
        .AddRef = tex_add_ref,
        .Release = tex_release,
        .LockRect = tex_lock_rect,
        .UnlockRect = tex_unlock_rect,
        .GetLevelDesc = tex_get_level_desc
    };
    texture->lpVtbl = &tex_vtbl;
    texture->texture = tex;
    texture->device = This;

    *ppTexture = texture;
    return D3D_OK;
}

static HRESULT D3DAPI d3d8_set_texture(IDirect3DDevice8 *This, DWORD Stage, IDirect3DTexture8 *pTexture) {
    if (Stage != 0) return D3DERR_INVALIDCALL;
    if (!pTexture) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        return D3D_OK;
    }
    glBindTexture(GL_TEXTURE_2D, pTexture->texture->tex_id);
    glEnable(GL_TEXTURE_2D);
    return D3D_OK;
}

static GLenum tex_arg_to_gl(DWORD arg) {
    switch (arg & D3DTA_SELECTMASK) {
        case D3DTA_DIFFUSE: return GL_PRIMARY_COLOR;
        case D3DTA_CURRENT: return GL_PREVIOUS;
        case D3DTA_TEXTURE: return GL_TEXTURE;
        default: return GL_TEXTURE;
    }
}

static HRESULT D3DAPI d3d8_set_texture_stage_state(IDirect3DDevice8 *This, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    if (Stage != 0) return D3DERR_INVALIDCALL;

    switch (Type) {
        case D3DTSS_COLOROP:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
            switch (Value) {
                case D3DTOP_MODULATE:
                    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
                    break;
                case D3DTOP_SELECTARG1:
                    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
                    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
                    break;
                case D3DTOP_SELECTARG2:
                    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
                    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
                    break;
                default:
                    return D3DERR_INVALIDCALL;
            }
            break;
        case D3DTSS_COLORARG1:
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, tex_arg_to_gl(Value));
            break;
        case D3DTSS_COLORARG2:
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, tex_arg_to_gl(Value));
            break;
        case D3DTSS_ALPHAOP:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
            switch (Value) {
                case D3DTOP_MODULATE:
                    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
                    break;
                case D3DTOP_SELECTARG1:
                    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
                    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
                    break;
                case D3DTOP_SELECTARG2:
                    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
                    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PREVIOUS);
                    break;
                default:
                    return D3DERR_INVALIDCALL;
            }
            break;
        case D3DTSS_ALPHAARG1:
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, tex_arg_to_gl(Value));
            break;
        case D3DTSS_ALPHAARG2:
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, tex_arg_to_gl(Value));
            break;
        case D3DTSS_TEXCOORDINDEX:
            if (Value > 1) return D3DERR_INVALIDCALL;
            This->gles->texcoord_index0 = Value;
            break;
        default:
            return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}

// D3DX functions
HRESULT WINAPI D3DXCreateBuffer(DWORD NumBytes, LPD3DXBUFFER *ppBuffer) {
    ID3DXBuffer *buffer = calloc(1, sizeof(ID3DXBuffer) + sizeof(ID3DXBufferVtbl));
    if (!buffer) return D3DERR_OUTOFVIDEOMEMORY;

    buffer->data = calloc(1, NumBytes);
    if (!buffer->data) {
        free(buffer);
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    buffer->size = NumBytes;

    static const ID3DXBufferVtbl buffer_vtbl = {
        .QueryInterface = d3dx_buffer_query_interface,
        .AddRef = d3dx_buffer_add_ref,
        .Release = d3dx_buffer_release,
        .GetBufferPointer = d3dx_buffer_get_buffer_pointer,
        .GetBufferSize = d3dx_buffer_get_buffer_size
    };
    buffer->pVtbl = &buffer_vtbl;

    *ppBuffer = buffer;
    return D3D_OK;
}

// ID3DXBuffer helpers
static HRESULT D3DAPI d3dx_buffer_query_interface(ID3DXBuffer *This, REFIID iid, void **ppv) {
    (void)This;
    (void)iid;
    (void)ppv;
    return D3DERR_NOTAVAILABLE;
}

static ULONG D3DAPI d3dx_buffer_add_ref(ID3DXBuffer *This) {
    (void)This;
    return 1;
}

static ULONG D3DAPI d3dx_buffer_release(ID3DXBuffer *This) {
    if (This) {
        free(This->data);
        free(This);
    }
    return 0;
}

static LPVOID d3dx_buffer_get_buffer_pointer(ID3DXBuffer *This) {
    return This ? This->data : NULL;
}

static DWORD d3dx_buffer_get_buffer_size(ID3DXBuffer *This) {
    return This ? This->size : 0;
}

UINT WINAPI D3DXGetFVFVertexSize(DWORD FVF) {
    if (!(FVF & (D3DFVF_XYZ | D3DFVF_XYZRHW))) return 0;

    if (FVF & ~(D3DFVF_XYZ | D3DFVF_XYZRHW | D3DFVF_NORMAL | D3DFVF_DIFFUSE |
                 D3DFVF_SPECULAR | D3DFVF_TEX1 | D3DFVF_TEX2))
        return 0;

    UINT size = (FVF & D3DFVF_XYZRHW) ? 4 * sizeof(float) : 3 * sizeof(float);
    if (FVF & D3DFVF_NORMAL) size += 3 * sizeof(float);
    if (FVF & D3DFVF_DIFFUSE) size += sizeof(DWORD);
    if (FVF & D3DFVF_SPECULAR) size += sizeof(DWORD);

    UINT tex_count = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    if (tex_count > 2) return 0;
    size += tex_count * 2 * sizeof(float); // each texcoord set is 2 floats
    return size;
}

HRESULT WINAPI D3DXDeclaratorFromFVF(DWORD FVF,
                                     DWORD Declaration[MAX_FVF_DECL_SIZE]) {
    if (!(FVF & (D3DFVF_XYZ | D3DFVF_XYZRHW))) return D3DERR_INVALIDCALL;
    if ((FVF & D3DFVF_XYZ) && (FVF & D3DFVF_XYZRHW)) return D3DERR_INVALIDCALL;
    if (FVF & ~(D3DFVF_XYZ | D3DFVF_XYZRHW | D3DFVF_NORMAL | D3DFVF_DIFFUSE |
                D3DFVF_SPECULAR | D3DFVF_TEX1 | D3DFVF_TEX2))
        return D3DERR_INVALIDCALL;

    int i = 0;
    Declaration[i++] = D3DVSD_STREAM(0);
    if (FVF & D3DFVF_XYZRHW)
        Declaration[i++] = D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT4);
    else
        Declaration[i++] = D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3);
    if (FVF & D3DFVF_NORMAL)
        Declaration[i++] = D3DVSD_REG(D3DVSDE_NORMAL, D3DVSDT_FLOAT3);
    if (FVF & D3DFVF_DIFFUSE)
        Declaration[i++] = D3DVSD_REG(D3DVSDE_DIFFUSE, D3DVSDT_D3DCOLOR);
    if (FVF & D3DFVF_SPECULAR)
        Declaration[i++] = D3DVSD_REG(D3DVSDE_SPECULAR, D3DVSDT_D3DCOLOR);

    UINT tex_count = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    if (tex_count > 0)
        Declaration[i++] = D3DVSD_REG(D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2);
    if (tex_count > 1)
        Declaration[i++] = D3DVSD_REG(D3DVSDE_TEXCOORD1, D3DVSDT_FLOAT2);
    Declaration[i++] = D3DVSD_END();

    for (; i < MAX_FVF_DECL_SIZE; i++) Declaration[i] = D3DVSD_END();
    return D3D_OK;
}

typedef struct {
    float x, y, z; // Position
    float nx, ny, nz; // Normal
} VertexPN;

HRESULT WINAPI D3DXCreateBox(LPDIRECT3DDEVICE8 pDevice, FLOAT Width, FLOAT Height, FLOAT Depth, LPD3DXMESH *ppMesh, LPD3DXBUFFER *ppAdjacency) {
    if (!pDevice || Width <= 0.0f || Height <= 0.0f || Depth <= 0.0f || !ppMesh) return D3DERR_INVALIDCALL;

    float hw = Width * 0.5f, hh = Height * 0.5f, hd = Depth * 0.5f;
    VertexPN vertices[] = {
        // Front face
        {-hw, -hh,  hd,  0,  0,  1}, {-hw,  hh,  hd,  0,  0,  1},
        { hw,  hh,  hd,  0,  0,  1}, { hw, -hh,  hd,  0,  0,  1},
        // Back face
        { hw, -hh, -hd,  0,  0, -1}, { hw,  hh, -hd,  0,  0, -1},
        {-hw,  hh, -hd,  0,  0, -1}, {-hw, -hh, -hd,  0,  0, -1},
        // Top face
        {-hw,  hh,  hd,  0,  1,  0}, {-hw,  hh, -hd,  0,  1,  0},
        { hw,  hh, -hd,  0,  1,  0}, { hw,  hh,  hd,  0,  1,  0},
        // Bottom face
        { hw, -hh,  hd,  0, -1,  0}, { hw, -hh, -hd,  0, -1,  0},
        {-hw, -hh, -hd,  0, -1,  0}, {-hw, -hh,  hd,  0, -1,  0},
        // Right face
        { hw, -hh,  hd,  1,  0,  0}, { hw,  hh,  hd,  1,  0,  0},
        { hw,  hh, -hd,  1,  0,  0}, { hw, -hh, -hd,  1,  0,  0},
        // Left face
        {-hw, -hh, -hd, -1,  0,  0}, {-hw,  hh, -hd, -1,  0,  0},
        {-hw,  hh,  hd, -1,  0,  0}, {-hw, -hh,  hd, -1,  0,  0}
    };

    WORD indices[] = {
        0, 1, 2, 0, 2, 3,       // Front
        4, 5, 6, 4, 6, 7,       // Back
        8, 9, 10, 8, 10, 11,    // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Right
        20, 21, 22, 20, 22, 23  // Left
    };

    DWORD num_vertices = 24;
    DWORD num_faces = 12;
    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL;
    UINT vb_size = num_vertices * sizeof(VertexPN);
    UINT ib_size = num_faces * 3 * sizeof(WORD);
    DWORD options = D3DXMESH_MANAGED;

    IDirect3DVertexBuffer8 *vb;
    HRESULT hr = pDevice->lpVtbl->CreateVertexBuffer(pDevice, vb_size, D3DUSAGE_WRITEONLY, fvf, D3DPOOL_MANAGED, &vb);
    if (FAILED(hr)) return hr;

    BYTE *vb_data;
    hr = vb->lpVtbl->Lock(vb, 0, vb_size, &vb_data, 0);
    if (SUCCEEDED(hr)) {
        memcpy(vb_data, vertices, vb_size);
        vb->lpVtbl->Unlock(vb);
    } else {
        vb->lpVtbl->Release(vb);
        return hr;
    }

    IDirect3DIndexBuffer8 *ib;
    hr = pDevice->lpVtbl->CreateIndexBuffer(pDevice, ib_size, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ib);
    if (FAILED(hr)) {
        vb->lpVtbl->Release(vb);
        return hr;
    }

    BYTE *ib_data;
    hr = ib->lpVtbl->Lock(ib, 0, ib_size, &ib_data, 0);
    if (SUCCEEDED(hr)) {
        memcpy(ib_data, indices, ib_size);
        ib->lpVtbl->Unlock(ib);
    } else {
        vb->lpVtbl->Release(vb);
        ib->lpVtbl->Release(ib);
        return hr;
    }

    ID3DXMesh *mesh = calloc(1, sizeof(ID3DXMesh) + sizeof(ID3DXMeshVtbl));
    if (!mesh) {
        vb->lpVtbl->Release(vb);
        ib->lpVtbl->Release(ib);
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    // Setup attribute table (single subset for simplicity)
    mesh->attrib_table = calloc(1, sizeof(D3DXATTRIBUTERANGE));
    if (!mesh->attrib_table) {
        vb->lpVtbl->Release(vb);
        ib->lpVtbl->Release(ib);
        free(mesh);
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    mesh->attrib_table[0].AttribId = 0;
    mesh->attrib_table[0].FaceStart = 0;
    mesh->attrib_table[0].FaceCount = num_faces;
    mesh->attrib_table[0].VertexStart = 0;
    mesh->attrib_table[0].VertexCount = num_vertices;
    mesh->attrib_table_size = 1;

    // Setup attribute buffer
    mesh->attrib_buffer = calloc(num_faces, sizeof(DWORD));
    if (!mesh->attrib_buffer) {
        vb->lpVtbl->Release(vb);
        ib->lpVtbl->Release(ib);
        free(mesh->attrib_table);
        free(mesh);
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    for (DWORD i = 0; i < num_faces; i++) {
        mesh->attrib_buffer[i] = 0; // Single material
    }

    static const ID3DXMeshVtbl mesh_vtbl = {
        .QueryInterface = d3dx_mesh_query_interface,
        .AddRef = d3dx_mesh_add_ref,
        .Release = d3dx_mesh_release,
        .DrawSubset = d3dx_mesh_draw_subset,
        .GetNumFaces = d3dx_mesh_get_num_faces,
        .GetNumVertices = d3dx_mesh_get_num_vertices,
        .GetFVF = d3dx_mesh_get_fvf,
        .GetDeclaration = d3dx_mesh_get_declaration,
        .GetOptions = d3dx_mesh_get_options,
        .GetDevice = d3dx_mesh_get_device,
        .CloneMeshFVF = d3dx_mesh_clone_mesh_fvf,
        .CloneMesh = d3dx_mesh_clone_mesh,
        .GetVertexBuffer = d3dx_mesh_get_vertex_buffer,
        .GetIndexBuffer = d3dx_mesh_get_index_buffer,
        .LockVertexBuffer = d3dx_mesh_lock_vertex_buffer,
        .UnlockVertexBuffer = d3dx_mesh_unlock_vertex_buffer,
        .LockIndexBuffer = d3dx_mesh_lock_index_buffer,
        .UnlockIndexBuffer = d3dx_mesh_unlock_index_buffer,
        .GetAttributeTable = d3dx_mesh_get_attribute_table,
        .ConvertPointRepsToAdjacency = d3dx_mesh_convert_point_reps_to_adjacency,
        .ConvertAdjacencyToPointReps = d3dx_mesh_convert_adjacency_to_point_reps,
        .GenerateAdjacency = d3dx_mesh_generate_adjacency,
        .LockAttributeBuffer = d3dx_mesh_lock_attribute_buffer,
        .UnlockAttributeBuffer = d3dx_mesh_unlock_attribute_buffer,
        .Optimize = d3dx_mesh_optimize,
        .OptimizeInplace = d3dx_mesh_optimize_inplace
    };
    mesh->pVtbl = &mesh_vtbl;
    mesh->device = pDevice;
    mesh->vb = vb;
    mesh->ib = ib;
    mesh->num_vertices = num_vertices;
    mesh->num_faces = num_faces;
    mesh->fvf = fvf;
    mesh->options = options;

    if (ppAdjacency) {
        hr = D3DXCreateBuffer(num_faces * 3 * sizeof(DWORD), ppAdjacency);
        if (SUCCEEDED(hr)) {
            DWORD *adj_data = (DWORD *)((*ppAdjacency)->pVtbl->GetBufferPointer(*ppAdjacency));
            memset(adj_data, 0xFFFFFFFF, num_faces * 3 * sizeof(DWORD)); // No adjacency for simplicity
        }
    }

    *ppMesh = mesh;
    return D3D_OK;
}

HRESULT WINAPI D3DXCreateMatrixStack(DWORD Flags, LPD3DXMATRIXSTACK *ppStack) {
    ID3DXMatrixStack *stack = calloc(1, sizeof(ID3DXMatrixStack) + sizeof(ID3DXMatrixStackVtbl));
    if (!stack) return D3DERR_OUTOFVIDEOMEMORY;

    static const ID3DXMatrixStackVtbl stack_vtbl = {
        .QueryInterface = d3dx_matrix_stack_query_interface,
        .AddRef = d3dx_matrix_stack_add_ref,
        .Release = d3dx_matrix_stack_release,
        .Pop = d3dx_matrix_stack_pop,
        .Push = d3dx_matrix_stack_push,
        .LoadIdentity = d3dx_matrix_stack_load_identity,
        .LoadMatrix = d3dx_matrix_stack_load_matrix,
        .MultMatrix = d3dx_matrix_stack_mult_matrix,
        .MultMatrixLocal = d3dx_matrix_stack_mult_matrix_local,
        .RotateAxis = d3dx_matrix_stack_rotate_axis,
        .RotateAxisLocal = d3dx_matrix_stack_rotate_axis_local,
        .RotateYawPitchRoll = d3dx_matrix_stack_rotate_yaw_pitch_roll,
        .RotateYawPitchRollLocal = d3dx_matrix_stack_rotate_yaw_pitch_roll_local,
        .Scale = d3dx_matrix_stack_scale,
        .ScaleLocal = d3dx_matrix_stack_scale_local,
        .Translate = d3dx_matrix_stack_translate,
        .TranslateLocal = d3dx_matrix_stack_translate_local,
        .GetTop = d3dx_matrix_stack_get_top
    };
    stack->pVtbl = &stack_vtbl;
    stack->capacity = 16;
    stack->stack = calloc(stack->capacity, sizeof(D3DXMATRIX));
    if (!stack->stack) {
        free(stack);
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    stack->top = 0;

    *ppStack = stack;
    return D3D_OK;
}

HRESULT WINAPI D3DXGetErrorStringA(HRESULT hr, LPSTR pBuffer, UINT BufferLen) {
    const char *msg = "Unknown error";
    switch (hr) {
        case D3D_OK: msg = "Success"; break;
        case D3DERR_INVALIDCALL: msg = "Invalid call"; break;
        case D3DERR_OUTOFVIDEOMEMORY: msg = "Out of video memory"; break;
        case D3DERR_NOTAVAILABLE: msg = "Not available"; break;
        case D3DXERR_INVALIDMESH: msg = "Invalid mesh"; break;
        case D3DXERR_SKINNINGNOTSUPPORTED: msg = "Skinning not supported"; break;
    }
    strncpy(pBuffer, msg, BufferLen);
    pBuffer[BufferLen - 1] = '\0';
    return D3D_OK;
}

// Additional math functions
D3DXMATRIX* WINAPI D3DXMatrixScaling(D3DXMATRIX *pOut, FLOAT sx, FLOAT sy, FLOAT sz) {
    D3DXMatrixIdentity(pOut);
    pOut->_11 = sx;
    pOut->_22 = sy;
    pOut->_33 = sz;
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixTranslation(D3DXMATRIX *pOut, FLOAT x, FLOAT y, FLOAT z) {
    D3DXMatrixIdentity(pOut);
    pOut->_41 = x;
    pOut->_42 = y;
    pOut->_43 = z;
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationX(D3DXMATRIX *pOut, FLOAT Angle) {
    float s = sinf(Angle), c = cosf(Angle);
    D3DXMatrixIdentity(pOut);
    pOut->_22 = c;
    pOut->_23 = s;
    pOut->_32 = -s;
    pOut->_33 = c;
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationY(D3DXMATRIX *pOut, FLOAT Angle) {
    float s = sinf(Angle), c = cosf(Angle);
    D3DXMatrixIdentity(pOut);
    pOut->_11 = c;
    pOut->_13 = -s;
    pOut->_31 = s;
    pOut->_33 = c;
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationZ(D3DXMATRIX *pOut, FLOAT Angle) {
    float s = sinf(Angle), c = cosf(Angle);
    D3DXMatrixIdentity(pOut);
    pOut->_11 = c;
    pOut->_12 = s;
    pOut->_21 = -s;
    pOut->_22 = c;
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationAxis(D3DXMATRIX *pOut, CONST D3DXVECTOR3 *pV, FLOAT Angle) {
    D3DXVECTOR3 axis;
    D3DXVec3Normalize(&axis, pV);
    float c = cosf(Angle), s = sinf(Angle), t = 1.0f - c;
    float x = axis.x, y = axis.y, z = axis.z;

    D3DXMatrixIdentity(pOut);
    pOut->_11 = t * x * x + c;
    pOut->_12 = t * x * y - s * z;
    pOut->_13 = t * x * z + s * y;
    pOut->_21 = t * x * y + s * z;
    pOut->_22 = t * y * y + c;
    pOut->_23 = t * y * z - s * x;
    pOut->_31 = t * x * z - s * y;
    pOut->_32 = t * y * z + s * x;
    pOut->_33 = t * z * z + c;
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationYawPitchRoll(D3DXMATRIX *pOut, FLOAT Yaw, FLOAT Pitch, FLOAT Roll) {
    D3DXMATRIX rotX, rotY, rotZ, temp;
    D3DXMatrixRotationX(&rotX, Pitch);
    D3DXMatrixRotationY(&rotY, Yaw);
    D3DXMatrixRotationZ(&rotZ, Roll);
    D3DXMatrixMultiply(&temp, &rotX, &rotY);
    D3DXMatrixMultiply(pOut, &temp, &rotZ);
    return pOut;
}

// Stubbed D3DX functions
HRESULT WINAPI D3DXCreatePolygon(LPDIRECT3DDEVICE8 pDevice, FLOAT Length, UINT Sides, LPD3DXMESH *ppMesh, LPD3DXBUFFER *ppAdjacency) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateCylinder(LPDIRECT3DDEVICE8 pDevice, FLOAT Radius1, FLOAT Radius2, FLOAT Length, UINT Slices, UINT Stacks, LPD3DXMESH *ppMesh, LPD3DXBUFFER *ppAdjacency) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateSphere(LPDIRECT3DDEVICE8 pDevice, FLOAT Radius, UINT Slices, UINT Stacks, LPD3DXMESH *ppMesh, LPD3DXBUFFER *ppAdjacency) {
    if (!pDevice || Radius <= 0.0f || Slices < 3 || Stacks < 2 || !ppMesh)
        return D3DERR_INVALIDCALL;

    DWORD num_vertices = (Stacks + 1) * (Slices + 1);
    DWORD num_faces = Stacks * Slices * 2;
    if (num_vertices > 0xFFFF)
        return D3DERR_INVALIDCALL;

    VertexPN *vertices = calloc(num_vertices, sizeof(VertexPN));
    WORD *indices = calloc(num_faces * 3, sizeof(WORD));
    if (!vertices || !indices) {
        free(vertices);
        free(indices);
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    const float pi = 3.14159265359f;
    DWORD v = 0;
    for (UINT i = 0; i <= Stacks; i++) {
        float phi = (float)i / (float)Stacks * pi;
        float y = cosf(phi);
        float r = sinf(phi);
        for (UINT j = 0; j <= Slices; j++) {
            float theta = (float)j / (float)Slices * 2.0f * pi;
            float x = r * cosf(theta);
            float z = r * sinf(theta);
            vertices[v].x = x * Radius;
            vertices[v].y = y * Radius;
            vertices[v].z = z * Radius;
            vertices[v].nx = x;
            vertices[v].ny = y;
            vertices[v].nz = z;
            v++;
        }
    }

    DWORD idx = 0;
    for (UINT i = 0; i < Stacks; i++) {
        for (UINT j = 0; j < Slices; j++) {
            WORD v0 = (WORD)(i * (Slices + 1) + j);
            WORD v1 = (WORD)((i + 1) * (Slices + 1) + j);
            WORD v2 = (WORD)(v0 + 1);
            WORD v3 = (WORD)(v1 + 1);
            indices[idx++] = v0;
            indices[idx++] = v1;
            indices[idx++] = v2;
            indices[idx++] = v2;
            indices[idx++] = v1;
            indices[idx++] = v3;
        }
    }

    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL;
    UINT vb_size = num_vertices * sizeof(VertexPN);
    UINT ib_size = num_faces * 3 * sizeof(WORD);
    DWORD options = D3DXMESH_MANAGED;

    IDirect3DVertexBuffer8 *vb;
    HRESULT hr = pDevice->lpVtbl->CreateVertexBuffer(pDevice, vb_size, D3DUSAGE_WRITEONLY, fvf, D3DPOOL_MANAGED, &vb);
    if (FAILED(hr)) {
        free(vertices);
        free(indices);
        return hr;
    }

    BYTE *vb_data;
    hr = vb->lpVtbl->Lock(vb, 0, vb_size, &vb_data, 0);
    if (SUCCEEDED(hr)) {
        memcpy(vb_data, vertices, vb_size);
        vb->lpVtbl->Unlock(vb);
    } else {
        vb->lpVtbl->Release(vb);
        free(vertices);
        free(indices);
        return hr;
    }

    IDirect3DIndexBuffer8 *ib;
    hr = pDevice->lpVtbl->CreateIndexBuffer(pDevice, ib_size, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ib);
    if (FAILED(hr)) {
        vb->lpVtbl->Release(vb);
        free(vertices);
        free(indices);
        return hr;
    }

    BYTE *ib_data;
    hr = ib->lpVtbl->Lock(ib, 0, ib_size, &ib_data, 0);
    if (SUCCEEDED(hr)) {
        memcpy(ib_data, indices, ib_size);
        ib->lpVtbl->Unlock(ib);
    } else {
        vb->lpVtbl->Release(vb);
        ib->lpVtbl->Release(ib);
        free(vertices);
        free(indices);
        return hr;
    }

    free(vertices);
    free(indices);

    ID3DXMesh *mesh = calloc(1, sizeof(ID3DXMesh) + sizeof(ID3DXMeshVtbl));
    if (!mesh) {
        vb->lpVtbl->Release(vb);
        ib->lpVtbl->Release(ib);
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    mesh->attrib_table = calloc(1, sizeof(D3DXATTRIBUTERANGE));
    if (!mesh->attrib_table) {
        vb->lpVtbl->Release(vb);
        ib->lpVtbl->Release(ib);
        free(mesh);
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    mesh->attrib_table[0].AttribId = 0;
    mesh->attrib_table[0].FaceStart = 0;
    mesh->attrib_table[0].FaceCount = num_faces;
    mesh->attrib_table[0].VertexStart = 0;
    mesh->attrib_table[0].VertexCount = num_vertices;
    mesh->attrib_table_size = 1;

    mesh->attrib_buffer = calloc(num_faces, sizeof(DWORD));
    if (!mesh->attrib_buffer) {
        vb->lpVtbl->Release(vb);
        ib->lpVtbl->Release(ib);
        free(mesh->attrib_table);
        free(mesh);
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    for (DWORD i = 0; i < num_faces; i++) mesh->attrib_buffer[i] = 0;

    static const ID3DXMeshVtbl mesh_vtbl = {
        .QueryInterface = d3dx_mesh_query_interface,
        .AddRef = d3dx_mesh_add_ref,
        .Release = d3dx_mesh_release,
        .DrawSubset = d3dx_mesh_draw_subset,
        .GetNumFaces = d3dx_mesh_get_num_faces,
        .GetNumVertices = d3dx_mesh_get_num_vertices,
        .GetFVF = d3dx_mesh_get_fvf,
        .GetDeclaration = d3dx_mesh_get_declaration,
        .GetOptions = d3dx_mesh_get_options,
        .GetDevice = d3dx_mesh_get_device,
        .CloneMeshFVF = d3dx_mesh_clone_mesh_fvf,
        .CloneMesh = d3dx_mesh_clone_mesh,
        .GetVertexBuffer = d3dx_mesh_get_vertex_buffer,
        .GetIndexBuffer = d3dx_mesh_get_index_buffer,
        .LockVertexBuffer = d3dx_mesh_lock_vertex_buffer,
        .UnlockVertexBuffer = d3dx_mesh_unlock_vertex_buffer,
        .LockIndexBuffer = d3dx_mesh_lock_index_buffer,
        .UnlockIndexBuffer = d3dx_mesh_unlock_index_buffer,
        .GetAttributeTable = d3dx_mesh_get_attribute_table,
        .ConvertPointRepsToAdjacency = d3dx_mesh_convert_point_reps_to_adjacency,
        .ConvertAdjacencyToPointReps = d3dx_mesh_convert_adjacency_to_point_reps,
        .GenerateAdjacency = d3dx_mesh_generate_adjacency,
        .LockAttributeBuffer = d3dx_mesh_lock_attribute_buffer,
        .UnlockAttributeBuffer = d3dx_mesh_unlock_attribute_buffer,
        .Optimize = d3dx_mesh_optimize,
        .OptimizeInplace = d3dx_mesh_optimize_inplace
    };

    mesh->pVtbl = &mesh_vtbl;
    mesh->device = pDevice;
    mesh->vb = vb;
    mesh->ib = ib;
    mesh->num_vertices = num_vertices;
    mesh->num_faces = num_faces;
    mesh->fvf = fvf;
    mesh->options = options;

    if (ppAdjacency) {
        hr = D3DXCreateBuffer(num_faces * 3 * sizeof(DWORD), ppAdjacency);
        if (SUCCEEDED(hr)) {
            DWORD *adj = (DWORD *)(*ppAdjacency)->pVtbl->GetBufferPointer(*ppAdjacency);
            memset(adj, 0xFFFFFFFF, num_faces * 3 * sizeof(DWORD));
        }
    }

    *ppMesh = mesh;
    return D3D_OK;
}
HRESULT WINAPI D3DXCreateTorus(LPDIRECT3DDEVICE8 pDevice, FLOAT InnerRadius, FLOAT OuterRadius, UINT Sides, UINT Rings, LPD3DXMESH *ppMesh, LPD3DXBUFFER *ppAdjacency) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateTeapot(LPDIRECT3DDEVICE8 pDevice, LPD3DXMESH *ppMesh, LPD3DXBUFFER *ppAdjacency) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateTextA(LPDIRECT3DDEVICE8 pDevice, HDC hDC, LPCSTR pText, FLOAT Deviation, FLOAT Extrusion, LPD3DXMESH *ppMesh, LPD3DXBUFFER *ppAdjacency, LPGLYPHMETRICSFLOAT pGlyphMetrics) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateFont(LPDIRECT3DDEVICE8 pDevice, HFONT hFont, LPD3DXFONT *ppFont) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateSprite(LPDIRECT3DDEVICE8 pDevice, LPD3DXSPRITE *ppSprite) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateRenderToSurface(LPDIRECT3DDEVICE8 pDevice, UINT Width, UINT Height, D3DFORMAT Format, BOOL DepthStencil, D3DFORMAT DepthStencilFormat, LPD3DXRENDERTOSURFACE *ppRenderToSurface) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateRenderToEnvMap(LPDIRECT3DDEVICE8 pDevice, UINT Size, D3DFORMAT Format, BOOL DepthStencil, D3DFORMAT DepthStencilFormat, LPD3DXRenderToEnvMap *ppRenderToEnvMap) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXAssembleShader(LPCVOID pSrcData, UINT SrcDataLen, DWORD Flags, LPD3DXBUFFER *ppConstants, LPD3DXBUFFER *ppCompiledShader, LPD3DXBUFFER *ppCompilationErrors) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateEffect(LPDIRECT3DDEVICE8 pDevice, LPCVOID pSrcData, UINT SrcDataSize, LPD3DXEFFECT *ppEffect, LPD3DXBUFFER *ppCompilationErrors) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateMesh(DWORD NumFaces, DWORD NumVertices, DWORD Options, CONST DWORD *pDeclaration, LPDIRECT3DDEVICE8 pD3D, LPD3DXMESH *ppMesh) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateMeshFVF(DWORD NumFaces, DWORD NumVertices, DWORD Options, DWORD FVF, LPDIRECT3DDEVICE8 pD3D, LPD3DXMESH *ppMesh) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateSPMesh(LPD3DXMESH pMesh, CONST DWORD *pAdjacency, CONST LPD3DXATTRIBUTEWEIGHTS pVertexAttributeWeights, CONST FLOAT *pVertexWeights, LPD3DXSPMESH *ppSMesh) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCleanMesh(LPD3DXMESH pMeshIn, CONST DWORD *pAdjacencyIn, LPD3DXMESH *ppMeshOut, DWORD *pAdjacencyOut, LPD3DXBUFFER *ppErrorsAndWarnings) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXValidMesh(LPD3DXMESH pMeshIn, CONST DWORD *pAdjacency, LPD3DXBUFFER *ppErrorsAndWarnings) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXGeneratePMesh(LPD3DXMESH pMesh, CONST DWORD *pAdjacency, CONST LPD3DXATTRIBUTEWEIGHTS pVertexAttributeWeights, CONST FLOAT *pVertexWeights, DWORD MinValue, DWORD Options, LPD3DXPMESH *ppPMesh) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXSimplifyMesh(LPD3DXMESH pMesh, CONST DWORD *pAdjacency, CONST LPD3DXATTRIBUTEWEIGHTS pVertexAttributeWeights, CONST FLOAT *pVertexWeights, DWORD MinValue, DWORD Options, LPD3DXMESH *ppMesh) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXComputeBoundingSphere(PVOID pPointsFVF, DWORD NumVertices, DWORD FVF, D3DXVECTOR3 *pCenter, FLOAT *pRadius) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXComputeBoundingBox(PVOID pPointsFVF, DWORD NumVertices, DWORD FVF, D3DXVECTOR3 *pMin, D3DXVECTOR3 *pMax) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXComputeNormals(LPD3DXBASEMESH pMesh, CONST DWORD *pAdjacency) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXLoadMeshFromX(LPSTR pFilename, DWORD Options, LPDIRECT3DDEVICE8 pD3D, LPD3DXBUFFER *ppAdjacency, LPD3DXBUFFER *ppMaterials, DWORD *pNumMaterials, LPD3DXMESH *ppMesh) { return D3DXERR_NOTAVAILABLE; }
HRESULT WINAPI D3DXCreateSkinMesh(DWORD NumFaces, DWORD NumVertices, DWORD NumBones, DWORD Options, CONST DWORD *pDeclaration, LPDIRECT3DDEVICE8 pD3D, LPD3DXSKINMESH *ppSkinMesh) { return D3DXERR_SKINNINGNOTSUPPORTED; }
HRESULT WINAPI D3DXCreateSkinMeshFVF(DWORD NumFaces, DWORD NumVertices, DWORD NumBones, DWORD Options, DWORD FVF, LPDIRECT3DDEVICE8 pD3D, LPD3DXSKINMESH *ppSkinMesh) { return D3DXERR_SKINNINGNOTSUPPORTED; }

// Direct3DCreate8
// IUnknown-style helpers for IDirect3D8
static HRESULT D3DAPI d3d8_query_interface(IDirect3D8 *This, REFIID riid, void **ppv) {
    return common_query_interface(This, riid, ppv);
}
static ULONG D3DAPI d3d8_add_ref(IDirect3D8 *This) { return common_add_ref(This); }
static ULONG D3DAPI d3d8_release(IDirect3D8 *This) { return common_release(This); }

IDirect3D8 *D3DAPI Direct3DCreate8(UINT SDKVersion) {
    if (SDKVersion != D3D_SDK_VERSION) {
        d3d8_gles_log("Invalid SDK version: %u\n", SDKVersion);
        return NULL;
    }

    IDirect3D8 *d3d = calloc(1, sizeof(IDirect3D8) + sizeof(IDirect3D8Vtbl));
    if (!d3d) return NULL;

    static const IDirect3D8Vtbl d3d_vtbl = {
        .QueryInterface = d3d8_query_interface,
        .AddRef = d3d8_add_ref,
        .Release = d3d8_release,
        .RegisterSoftwareDevice = d3d8_register_software_device,
        .GetAdapterCount = d3d8_get_adapter_count,
        .GetAdapterIdentifier = d3d8_get_adapter_identifier,
        .GetAdapterModeCount = d3d8_get_adapter_mode_count,
        .EnumAdapterModes = d3d8_enum_adapter_modes,
        .GetAdapterDisplayMode = d3d8_get_adapter_display_mode,
        .CheckDeviceType = d3d8_check_device_type,
        .CheckDeviceFormat = d3d8_check_device_format,
        .CheckDeviceMultiSampleType = d3d8_check_device_multi_sample_type,
        .CheckDepthStencilMatch = d3d8_check_depth_stencil_match,
        .GetDeviceCaps = d3d8_get_device_caps,
        .GetAdapterMonitor = d3d8_get_adapter_monitor,
        .CreateDevice = d3d8_create_device
    };
    d3d->lpVtbl = &d3d_vtbl;

    return d3d;
}