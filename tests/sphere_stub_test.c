#include <assert.h>
#include <d3d8_to_gles.h>

int main(void) {
    /* Invalid parameter checks */
    HRESULT hr = D3DXCreateSphere(NULL, 1.0f, 8, 8, NULL, NULL);
    assert(hr == D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, -1.0f, 8, 8, NULL, NULL);
    assert(hr == D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, 1.0f, 2, 8, NULL, NULL);
    assert(hr == D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, 1.0f, 8, 1, NULL, NULL);
    assert(hr == D3DERR_INVALIDCALL);

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
    hr = d3d->lpVtbl->CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, pp.hDeviceWindow, 0, &pp, &device);
    assert(hr == D3D_OK && "CreateDevice failed");

    ID3DXMesh *mesh = NULL;
    hr = D3DXCreateSphere(device, 1.0f, 8, 8, &mesh, NULL);
    assert(hr == D3D_OK && mesh && "D3DXCreateSphere failed");

    mesh->pVtbl->Release(mesh);
    device->lpVtbl->Release(device);
    d3d->lpVtbl->Release(d3d);
    return 0;
}
