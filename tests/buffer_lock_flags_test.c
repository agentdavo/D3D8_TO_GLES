#include <assert.h>
#include <d3d8_to_gles.h>
#include <string.h>

typedef struct {
    float x, y, z;
} Vertex;

int main(void) {
    IDirect3D8 *d3d = Direct3DCreate8(D3D_SDK_VERSION);
    assert(d3d && "Failed to create D3D8 interface");

    D3DPRESENT_PARAMETERS pp = {0};
    pp.BackBufferWidth = 8;
    pp.BackBufferHeight = 8;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = 0;
    pp.Windowed = TRUE;
    pp.EnableAutoDepthStencil = FALSE;
    pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    IDirect3DDevice8 *device = NULL;
    HRESULT hr =
        d3d->lpVtbl->CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                   pp.hDeviceWindow, 0, &pp, &device);
    assert(hr == D3D_OK && device);

    UINT stride = sizeof(Vertex);
    IDirect3DVertexBuffer8 *vb = NULL;
    hr = device->lpVtbl->CreateVertexBuffer(device, 6 * stride,
                                            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                            D3DFVF_XYZ, D3DPOOL_MANAGED, &vb);
    assert(hr == D3D_OK && vb);

    Vertex verts[6] = {{0}};
    BYTE *ptr;
    hr = vb->lpVtbl->Lock(vb, 0, 3 * stride, &ptr, D3DLOCK_DISCARD);
    assert(hr == D3D_OK);
    memcpy(ptr, verts, 3 * stride);
    vb->lpVtbl->Unlock(vb);

    hr = vb->lpVtbl->Lock(vb, 3 * stride, 3 * stride, &ptr, D3DLOCK_NOOVERWRITE);
    assert(hr == D3D_OK);
    memcpy(ptr, verts, 3 * stride);
    vb->lpVtbl->Unlock(vb);

    IDirect3DIndexBuffer8 *ib = NULL;
    hr = device->lpVtbl->CreateIndexBuffer(device, 6 * sizeof(WORD),
                                           D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                           D3DFMT_INDEX16, D3DPOOL_MANAGED, &ib);
    assert(hr == D3D_OK && ib);

    WORD indices[6] = {0};
    hr = ib->lpVtbl->Lock(ib, 0, 3 * sizeof(WORD), &ptr, D3DLOCK_DISCARD);
    assert(hr == D3D_OK);
    memcpy(ptr, indices, 3 * sizeof(WORD));
    ib->lpVtbl->Unlock(ib);

    hr = ib->lpVtbl->Lock(ib, 3 * sizeof(WORD), 3 * sizeof(WORD), &ptr,
                          D3DLOCK_NOOVERWRITE);
    assert(hr == D3D_OK);
    memcpy(ptr, indices, 3 * sizeof(WORD));
    ib->lpVtbl->Unlock(ib);

    ib->lpVtbl->Release(ib);
    vb->lpVtbl->Release(vb);
    device->lpVtbl->Release(device);
    d3d->lpVtbl->Release(d3d);
    return 0;
}
