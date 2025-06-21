#include <assert.h>
#include <d3d8_to_gles.h>

/* Prototype is not declared in the public header */
HRESULT WINAPI D3DXCreateCylinder(LPDIRECT3DDEVICE8 pDevice,
                                  FLOAT Radius1,
                                  FLOAT Radius2,
                                  FLOAT Length,
                                  UINT Slices,
                                  UINT Stacks,
                                  LPD3DXMESH *ppMesh,
                                  LPD3DXBUFFER *ppAdjacency);

int main(void) {
    HRESULT hr = D3DXCreateCylinder(NULL, 1.0f, 1.0f, 1.0f, 8, 1, NULL, NULL);
    assert(hr == D3DXERR_NOTAVAILABLE);
    return 0;
}
