#include <assert.h>
#include <d3d8_to_gles.h>
#include <string.h>

int main(void) {
    IDirect3D8 *d3d = Direct3DCreate8(D3D_SDK_VERSION);
    assert(d3d && "Failed to create D3D8 interface");

    D3DPRESENT_PARAMETERS pp = {0};
    pp.BackBufferWidth = 2;
    pp.BackBufferHeight = 1;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = 0;
    pp.Windowed = TRUE;
    pp.EnableAutoDepthStencil = TRUE;
    pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    IDirect3DDevice8 *device = NULL;
    HRESULT hr = d3d->lpVtbl->CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                           pp.hDeviceWindow, 0, &pp, &device);
    assert(hr == D3D_OK && "CreateDevice failed");

    /* First pass: write stencil value 1 on left half */
    hr = device->lpVtbl->SetRenderState(device, D3DRS_STENCILENABLE, TRUE);
    assert(hr == D3D_OK);
    hr = device->lpVtbl->SetRenderState(device, D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
    assert(hr == D3D_OK);
    hr = device->lpVtbl->SetRenderState(device, D3DRS_STENCILREF, 1);
    assert(hr == D3D_OK);
    hr = device->lpVtbl->SetRenderState(device, D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
    assert(hr == D3D_OK);
    hr = device->lpVtbl->SetRenderState(device, D3DRS_STENCILMASK, 0xFF);
    assert(hr == D3D_OK);
    hr = device->lpVtbl->SetRenderState(device, D3DRS_STENCILWRITEMASK, 0xFF);
    assert(hr == D3D_OK);
    hr = device->lpVtbl->SetRenderState(device, D3DRS_COLORWRITEENABLE, 0);
    assert(hr == D3D_OK);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    GLfloat mask_quad[] = {
        -1.0f, -1.0f, 0.0f,
         0.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         0.0f,  1.0f, 0.0f,
    };
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, mask_quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    /* Second pass: draw full quad with stencil test equal to 1 */
    hr = device->lpVtbl->SetRenderState(device, D3DRS_COLORWRITEENABLE,
                                        D3DCOLORWRITEENABLE_RED |
                                        D3DCOLORWRITEENABLE_GREEN |
                                        D3DCOLORWRITEENABLE_BLUE |
                                        D3DCOLORWRITEENABLE_ALPHA);
    assert(hr == D3D_OK);
    hr = device->lpVtbl->SetRenderState(device, D3DRS_STENCILFUNC, D3DCMP_EQUAL);
    assert(hr == D3D_OK);
    hr = device->lpVtbl->SetRenderState(device, D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    assert(hr == D3D_OK);

    GLfloat full_quad[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
    };
    glVertexPointer(3, GL_FLOAT, 0, full_quad);
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    unsigned char pixels[8] = {0};
    glReadPixels(0, 0, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    assert(pixels[1] == 255 && pixels[0] == 0 && pixels[2] == 0); /* left pixel green */
    assert(pixels[4] == 0 && pixels[5] == 0 && pixels[6] == 0);   /* right pixel black */

    glDisableClientState(GL_VERTEX_ARRAY);
    device->lpVtbl->Release(device);
    d3d->lpVtbl->Release(d3d);
    return 0;
}
