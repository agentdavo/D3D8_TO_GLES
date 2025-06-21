#include <assert.h>
#include <d3d8_to_gles.h>
#include <string.h>

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

    D3DADAPTER_IDENTIFIER8 ident;
    hr = d3d->lpVtbl->GetAdapterIdentifier(d3d, D3DADAPTER_DEFAULT, 0, &ident);
    assert(hr == D3D_OK && "GetAdapterIdentifier failed");

    const char *vendor = (const char *)glGetString(GL_VENDOR);
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    assert(vendor && renderer && "glGetString failed");

    assert(strcmp(ident.Description, vendor) == 0);
    assert(strcmp(ident.Driver, renderer) == 0);

    D3DDISPLAYMODE mode;
    hr = d3d->lpVtbl->EnumAdapterModes(d3d, D3DADAPTER_DEFAULT, 0, &mode);
    assert(hr == D3D_OK && "EnumAdapterModes failed");
    assert(mode.Width == pp.BackBufferWidth);
    assert(mode.Height == pp.BackBufferHeight);
    assert(mode.Format == pp.BackBufferFormat);
    assert(mode.RefreshRate == pp.FullScreen_RefreshRateInHz);

    device->lpVtbl->Release(device);
    d3d->lpVtbl->Release(d3d);
    return 0;
}
