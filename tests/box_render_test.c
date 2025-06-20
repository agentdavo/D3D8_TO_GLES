#include <assert.h>
#include <d3d8_to_gles.h>
#include <math.h>

int main(void) {
  IDirect3D8 *d3d = Direct3DCreate8(D3D_SDK_VERSION);
  assert(d3d && "Failed to create D3D8 interface");

  D3DPRESENT_PARAMETERS pp = {0};
  pp.BackBufferWidth = 64;
  pp.BackBufferHeight = 64;
  pp.BackBufferFormat = D3DFMT_X8R8G8B8;
  pp.BackBufferCount = 1;
  pp.MultiSampleType = D3DMULTISAMPLE_NONE;
  pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  pp.hDeviceWindow = 0;
  pp.Windowed = TRUE;
  pp.EnableAutoDepthStencil = FALSE;
  pp.Flags = 0;
  pp.FullScreen_RefreshRateInHz = 0;
  pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

  IDirect3DDevice8 *device = NULL;
  HRESULT hr =
      d3d->lpVtbl->CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                pp.hDeviceWindow, 0, &pp, &device);
  assert(hr == D3D_OK && "CreateDevice failed");

  ID3DXMesh *mesh = NULL;
  hr = D3DXCreateBox(device, 1.0f, 1.0f, 1.0f, &mesh, NULL);
  assert(hr == D3D_OK && "D3DXCreateBox failed");

  D3DXMATRIX view, proj;
  D3DXVECTOR3 eye = {0.0f, 0.0f, -5.0f};
  D3DXVECTOR3 at = {0.0f, 0.0f, 0.0f};
  D3DXVECTOR3 up = {0.0f, 1.0f, 0.0f};
  D3DXMatrixLookAtLH(&view, &eye, &at, &up);
  hr = device->lpVtbl->SetTransform(device, D3DTS_VIEW, &view);
  assert(hr == D3D_OK && "SetTransform view failed");

  D3DXMatrixPerspectiveFovLH(&proj, (FLOAT)(45.0 * (M_PI / 180.0)), 1.0f, 1.0f,
                             10.0f);
  hr = device->lpVtbl->SetTransform(device, D3DTS_PROJECTION, &proj);
  assert(hr == D3D_OK && "SetTransform projection failed");

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
