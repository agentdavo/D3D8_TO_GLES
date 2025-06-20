#include <assert.h>
#include <d3d8_to_gles.h>

int main(void) {
    IDirect3D8 *d3d = Direct3DCreate8(D3D_SDK_VERSION);
    assert(d3d && "Failed to create D3D8 interface");

    D3DCAPS8 caps;
    HRESULT hr = d3d->lpVtbl->GetDeviceCaps(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    assert(hr == D3D_OK && "GetDeviceCaps failed");

    /* Check texture capabilities */
    assert(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP);
    /* Check for basic filter caps */
    assert((caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR) &&
           (caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR));
    /* Check rasterization capability */
    assert(caps.DevCaps & D3DDEVCAPS_HWRASTERIZATION);
    /* Confirm depth test support */
    assert(caps.RasterCaps & D3DPRASTERCAPS_ZTEST);

    d3d->lpVtbl->Release(d3d);
    return 0;
}
