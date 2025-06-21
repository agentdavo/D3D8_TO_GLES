#include <assert.h>
#include <d3d8_to_gles.h>

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
    HRESULT hr = d3d->lpVtbl->CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, pp.hDeviceWindow, 0, &pp, &device);
    assert(hr == D3D_OK && "CreateDevice failed");

    IDirect3DTexture8 *tex = NULL;
    hr = device->lpVtbl->CreateTexture(device, 4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex);
    assert(hr == D3D_OK && "CreateTexture failed");

    hr = device->lpVtbl->SetTexture(device, 0, tex);
    assert(hr == D3D_OK && "SetTexture failed");

    hr = device->lpVtbl->SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    assert(hr == D3D_OK && "SetTextureStageState COLOROP failed");

    hr = device->lpVtbl->SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    assert(hr == D3D_OK && "SetTextureStageState COLORARG1 failed");

    hr = device->lpVtbl->SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_MODULATE);
    assert(hr == D3DERR_INVALIDCALL && "Stage1 should fail");

    tex->lpVtbl->Release(tex);
    device->lpVtbl->Release(device);
    d3d->lpVtbl->Release(d3d);
    return 0;
}
