#include <assert.h>
#include <d3d8_to_gles.h>

int main(void) {
  HRESULT hr = D3DXCreateSphere(NULL, 1.0f, 8, 8, NULL, NULL);
  assert(hr == D3DXERR_NOTAVAILABLE);
  return 0;
}
