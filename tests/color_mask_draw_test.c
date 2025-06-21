#include <assert.h>
#include <d3d8_to_gles.h>
#include <string.h>

int main(void) {
    IDirect3D8 *d3d = Direct3DCreate8(D3D_SDK_VERSION);
    assert(d3d && "Failed to create D3D8 interface");

    D3DPRESENT_PARAMETERS pp = {0};
    pp.BackBufferWidth = 1;
    pp.BackBufferHeight = 1;
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

    /* Clear the buffer to black with all channels enabled */
    hr = device->lpVtbl->SetRenderState(device, D3DRS_COLORWRITEENABLE,
                                        D3DCOLORWRITEENABLE_RED |
                                        D3DCOLORWRITEENABLE_GREEN |
                                        D3DCOLORWRITEENABLE_BLUE |
                                        D3DCOLORWRITEENABLE_ALPHA);
    assert(hr == D3D_OK);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Enable only red writes and clear to white */
    hr = device->lpVtbl->SetRenderState(device, D3DRS_COLORWRITEENABLE,
                                        D3DCOLORWRITEENABLE_RED);
    assert(hr == D3D_OK);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Restore mask and verify pixel value */
    hr = device->lpVtbl->SetRenderState(device, D3DRS_COLORWRITEENABLE,
                                        D3DCOLORWRITEENABLE_RED |
                                        D3DCOLORWRITEENABLE_GREEN |
                                        D3DCOLORWRITEENABLE_BLUE |
                                        D3DCOLORWRITEENABLE_ALPHA);
    assert(hr == D3D_OK);
    unsigned char pixel[4] = {0};
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    assert(pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 0);

    device->lpVtbl->Release(device);
    d3d->lpVtbl->Release(d3d);
    return 0;
}
