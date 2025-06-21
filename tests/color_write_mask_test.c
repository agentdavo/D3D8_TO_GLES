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
    HRESULT hr = d3d->lpVtbl->CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                           pp.hDeviceWindow, 0, &pp, &device);
    assert(hr == D3D_OK && "CreateDevice failed");

    GLboolean mask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, mask);
    assert(mask[0] && mask[1] && mask[2] && mask[3]);

    hr = device->lpVtbl->SetRenderState(device, D3DRS_COLORWRITEENABLE,
                                        D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_ALPHA);
    assert(hr == D3D_OK && "SetRenderState RED|ALPHA failed");
    glGetBooleanv(GL_COLOR_WRITEMASK, mask);
    assert(mask[0] && !mask[1] && !mask[2] && mask[3]);

    hr = device->lpVtbl->SetRenderState(device, D3DRS_COLORWRITEENABLE,
                                        D3DCOLORWRITEENABLE_GREEN);
    assert(hr == D3D_OK && "SetRenderState GREEN failed");
    glGetBooleanv(GL_COLOR_WRITEMASK, mask);
    assert(!mask[0] && mask[1] && !mask[2] && !mask[3]);

    device->lpVtbl->Release(device);
    d3d->lpVtbl->Release(d3d);
    return 0;
}
