#include <d3d8_to_gles.h>
#include <stdio.h>

int main(void) {
    IDirect3D8 *d3d = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d) {
        printf("Failed to create D3D8 interface\n");
        return 1;
    }

    D3DCAPS8 caps;
    if (d3d->lpVtbl->GetDeviceCaps(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps) != D3D_OK) {
        printf("GetDeviceCaps failed\n");
        d3d->lpVtbl->Release(d3d);
        return 1;
    }

    printf("MaxTextureWidth=%lu\n", (unsigned long)caps.MaxTextureWidth);
    printf("MaxTextureHeight=%lu\n", (unsigned long)caps.MaxTextureHeight);

    d3d->lpVtbl->Release(d3d);
    return 0;
}
