#include <assert.h>
#include <d3d8_to_gles.h>
#include <GLES/gl.h>

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

    GLboolean enabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
    assert(!enabled);

    hr = device->lpVtbl->SetRenderState(device, D3DRS_ZBIAS, 2);
    assert(hr == D3D_OK && "SetRenderState ZBIAS failed");
    enabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
    assert(enabled);

    GLfloat val;
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &val);
    assert((int)val == 2);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &val);
    assert((int)val == 2);

    hr = device->lpVtbl->SetRenderState(device, D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    assert(hr == D3D_OK && "SetRenderState FILLMODE failed");

    ID3DXMesh *mesh = NULL;
    hr = D3DXCreateBox(device, 1.0f, 1.0f, 1.0f, &mesh, NULL);
    assert(hr == D3D_OK && "D3DXCreateBox failed");

    hr = device->lpVtbl->BeginScene(device);
    assert(hr == D3D_OK && "BeginScene failed");
    hr = mesh->pVtbl->DrawSubset(mesh, 0);
    assert(hr == D3D_OK && "DrawSubset failed");
    hr = device->lpVtbl->EndScene(device);
    assert(hr == D3D_OK && "EndScene failed");

    mesh->pVtbl->Release(mesh);
    device->lpVtbl->Release(device);
    d3d->lpVtbl->Release(d3d);
    return 0;
}
