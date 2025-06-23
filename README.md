# DirectX 8 to OpenGL ES 1.1 Shim

This project provides a lightweight shim to translate DirectX 8 (D3D8) API calls to OpenGL ES 1.1, enabling legacy D3D8 applications to run on platforms supporting OpenGL ES 1.1, such as embedded systems. The shim focuses on core rendering, mesh handling, and D3DX math utilities, mapping D3D8’s left-handed coordinate system to OpenGL’s right-handed system.

## Features
- Implements key D3D8 interfaces: `IDirect3D8`, `IDirect3DDevice8`, `IDirect3DVertexBuffer8`, `IDirect3DIndexBuffer8`.
- Supports D3DX utilities: `ID3DXMesh`, `ID3DXMatrixStack`, shape helpers `D3DXCreateBox` and `D3DXCreateSphere`, and matrix/vector operations (`D3DXMatrix*`, `D3DXVec3*`).
- Handles rendering with `DrawIndexedPrimitive` using OpenGL ES 1.1’s fixed-function pipeline.
- Converts D3D8 transformations to OpenGL ES 1.1 format, ensuring correct coordinate system handling.
- Portable C11 implementation with minimal dependencies (OpenGL ES 1.1, EGL, standard C libraries).

## Limitations
- No support for programmable shaders (`ID3DXEffect`, pixel/vertex shaders) due to OpenGL ES 1.1’s fixed-function pipeline.
- Skinning (`ID3DXSkinMesh`) and advanced mesh operations (`D3DXGeneratePMesh`) are not implemented.
- Texture support (`IDirect3DTexture8`, `ID3DXSprite`) and file I/O (`D3DXLoadMeshFromX`) are stubbed.
- Limited FVF support (`D3DFVF_XYZ`, `D3DFVF_NORMAL`); additional components (e.g., `D3DFVF_TEX1`) require extension.

## Building
### Prerequisites
- CMake 3.10 or higher
- OpenGL ES 1.1 and EGL libraries (or desktop OpenGL for testing)
- C11-compatible compiler (e.g., GCC, Clang)

### Instructions
```bash
mkdir build
cd build
cmake ..
make
```

To enable debug logging, set the `ENABLE_LOGGING` option:
```bash
cmake -DENABLE_LOGGING=ON ..
```

The build produces a static library (`libd3d8_to_gles.a`) in `build/`.

## Usage
1. Link `libd3d8_to_gles.a` to your application.
2. Include `d3d8_to_gles.h` and call D3D8 APIs as in a standard DirectX 8 application.
3. Example:
   ```c
   #include <d3d8_to_gles.h>
   int main() {
       IDirect3D8 *d3d = Direct3DCreate8(D3D_SDK_VERSION);
       D3DPRESENT_PARAMETERS pp = { /* configure */ };
       IDirect3DDevice8 *device;
   d3d->lpVtbl->CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, 0, &pp, &device);
       // Set transforms, create mesh, render
   d3d->lpVtbl->Release(d3d);
   return 0;
}
```

Passing `NULL` for the window handle lets the shim create an offscreen EGL
pbuffer surface, which is useful when running unit tests or headless tools.

## Directory Structure
```
project_root/
├── include/
│   └── d3d8_to_gles.h   # Interface definitions and function prototypes
├── src/
│   └── d3d8_to_gles.c   # Core shim implementation
├── lib/
│   └── min-dx8-sdk/     # Reference DirectX 8 headers (not used for build)
├── CMakeLists.txt       # Build configuration
├── AGENTS.md            # Guidance for AI code agents
```

The `lib/min-dx8-sdk` directory includes a trimmed copy of Microsoft’s
DirectX 8 SDK headers. These files are provided solely for reference when
developing or verifying the shim’s interfaces. The shim is compiled only
against the headers in `include/` and does **not** use the SDK headers when
building.

## Tests
Unit tests reside in the `tests/` directory and can be executed with `ctest`.
Recent additions cover color write masks via `D3DRS_COLORWRITEENABLE` and
basic stencil operations with a masked draw.

## Contributing
See `AGENTS.md` for guidance on extending the shim, especially for AI-assisted contributions. Key areas for improvement:
- Implement additional D3DX shapes (e.g., `D3DXCreateSphere`).
- Add texture support (`IDirect3DTexture8`, `ID3DXSprite`).
- Expand FVF component handling in `setup_vertex_attributes`.
- Develop test cases in `tests/fixtures` (planned for future versions).

Contributions must adhere to C11, avoid external dependencies, and compile on both desktop GL stubs and real OpenGL ES 1.1 hardware.

## License
This project is licensed under the MIT License. See `LICENSE` for details (to be added).

## Acknowledgments
- Built with reference to Microsoft’s DirectX 8 SDK headers (`d3d8.h`, `d3dx8*.h`).
- Designed for portability and compatibility with embedded systems supporting OpenGL ES 1.1.