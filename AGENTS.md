# Guidance for LLM / AI Code Agents

This document provides autonomous or semi-autonomous coders (GitHub Copilot, ChatGPT Code Interpreter, etc.) the *minimum context* needed to reason about and safely extend the **DirectX 8 to OpenGL ES 1.1 Shim** codebase (`d3d8_to_gles.c`).

## 1. High-Level Architecture

The shim translates DirectX 8 (D3D8) API calls to OpenGL ES 1.1, focusing on core rendering, mesh handling, and math operations. The codebase consists of:

1. **Core Interface** (`include/d3d8_to_gles.h`, `src/d3d8_to_gles.c`):
   - Implements `IDirect3D8`, `IDirect3DDevice8`, `IDirect3DVertexBuffer8`, `IDirect3DIndexBuffer8`, `ID3DXMesh`, and `ID3DXMatrixStack`.
   - Maps D3D8 calls (e.g., `CreateDevice`, `DrawIndexedPrimitive`) to OpenGL ES 1.1 and EGL.

2. **D3DX Utilities** (`src/d3d8_to_gles.c`):
   - Supports D3DX functions like `D3DXCreateBox`, `D3DXMatrix*`, and `D3DXVec3*`.
   - Handles matrix transformations and mesh creation, converting D3D8’s left-handed coordinate system to OpenGL’s right-handed system.

3. **Rendering Pipeline**:
   - Manages vertex/index buffers, transformations (`SetTransform`), and rendering (`DrawIndexedPrimitive`) using OpenGL ES 1.1’s fixed-function pipeline.
   - Converts D3D8 matrices to OpenGL format via `d3d_to_gl_matrix`.

```
D3D8 API Call ─┐
               ▼            +──────────────+     +─────────────────+
               CreateDevice ─▶ GLES_Device ──▶ OpenGL ES 1.1 Setup
               SetTransform    | state       |     | (EGL, matrices, |
               DrawIndexed     +──────────────+     |  vertex arrays) |
               D3DXCreateBox                        +─────────────────+
```

## 2. Adding a New Feature

1. **New D3D8 Interface Method**:
   - Update `include/d3d8_to_gles.h` to add the method to the relevant interface (e.g., `IDirect3DDevice8Vtbl`).
   - Implement the method in `src/d3d8_to_gles.c`, mapping to OpenGL ES 1.1 calls or returning `D3DERR_NOTAVAILABLE` for unsupported features.
   - Example for `SetTexture`:
     ```c
     static HRESULT D3DAPI d3d8_set_texture(IDirect3DDevice8 *This, DWORD Stage, IDirect3DBaseTexture8 *pTexture) {
         if (!pTexture) {
             glDisable(GL_TEXTURE_2D);
             return D3D_OK;
         }
         // Map texture to GL_TEXTURE_2D (requires IDirect3DTexture8 implementation)
         return D3DERR_NOTAVAILABLE;
     }
     ```

2. **New D3DX Function**:
   - Add the function prototype to `include/d3d8_to_gles.h` (e.g., `D3DXCreateSphere`).
   - Implement in `src/d3d8_to_gles.c`, creating vertex/index buffers and updating `ID3DXMesh`.
   - Example for `D3DXCreateSphere`:
     ```c
     HRESULT WINAPI D3DXCreateSphere(LPDIRECT3DDEVICE8 pDevice, FLOAT Radius, UINT Slices, UINT Stacks, LPD3DXMESH *ppMesh, LPD3DXBUFFER *ppAdjacency) {
         // Generate vertices and indices for sphere
         // Create vertex/index buffers and ID3DXMesh
         return D3DXERR_NOTAVAILABLE; // Stub for now
     }
     ```

3. **Coordinate System Conversion**:
   - Ensure matrix transformations use `d3d_to_gl_matrix` to handle left-handed (D3D8, z=[0,1]) to right-handed (OpenGL, z=[-1,1]) conversion.
   - Example:
     ```c
     GLfloat gl_matrix[16];
     d3d_to_gl_matrix(gl_matrix, &This->gles->world_matrix);
     glMatrixMode(GL_MODELVIEW);
     glLoadMatrixf(gl_matrix);
     ```

4. **Keep Implementations Simple**:
   - Avoid complex logic in inner loops; prefer straightforward mappings to OpenGL ES 1.1 calls.
   - If adding many new methods, consider a lookup table for method dispatching to reduce `if` branches.

## 3. Coding Standards

- **C11**, portable CMake, minimal dependencies (only OpenGL ES 1.1, EGL, and standard C `math.h`, `stdio.h`).
- No C++ code—strictly C11, as specified in `CMakeLists.txt` (`LANGUAGES C`).
- No external libraries (e.g., zlib, stb) without human review.
- **Do not** include the reference headers in `lib/min-dx8-sdk` when compiling;
  the shim builds solely against the headers in `include/`.
- Use `calloc`/`free` for memory management; avoid dynamic linking (static `.a` only for embedded compatibility).
- Follow `clang-format` defaults: 4-space indent, 100-column width.
- Logging controlled by `D3D8_GLES_LOGGING` macro; use `d3d8_gles_log` for debug output.

## 4. Error Handling

- Functions return `D3D_OK` (0) on success, negative HRESULTs (e.g., `D3DERR_INVALIDCALL`, `D3DERR_OUTOFVIDEOMEMORY`) on failure.
- Use `D3DXGetErrorStringA` to provide human-readable error descriptions; update it for new error codes.
- Avoid returning heap pointers to callers; store error details in temporary buffers or static strings.

## 5. Future Automation Ideas

- **Test Generation**:
  - Create test cases in `tests/fixtures` that call D3D8 APIs (e.g., `D3DXCreateBox`, `DrawIndexedPrimitive`) and verify OpenGL ES 1.1 output via JSON dumps or reference images.
  - Example: Render a box mesh and compare the GL state (matrices, vertex arrays) against expected values.

- **Matrix Optimization**:
  - Explore peephole optimizations in `d3d_to_gl_matrix`, e.g., skip redundant matrix multiplications if the world matrix is identity.
  - Example: `if (D3DXMatrixIsIdentity(&This->gles->world_matrix)) { /* skip */ }`.

- **Doc Bot**:
  - Auto-update `README.md` with a table of supported D3D8/D3DX functions when new methods are added to `d3d8_to_gles.c`.
  - Example: Document `D3DXCreateSphere` support with input parameters and limitations.

- **FVF Expansion**:
  - Automatically extend `setup_vertex_attributes` to handle additional FVF components (e.g., `D3DFVF_DIFFUSE`, `D3DFVF_TEX1`) based on `d3d8types.h`.

## 6. Known Limitations / Non-Goals

- **No Programmable Shaders**: OpenGL ES 1.1 uses a fixed-function pipeline, so `ID3DXEffect`, pixel shaders, and vertex shaders (beyond FVF) are not supported. Vertex shader emulation (CPU-based) is a future possibility.
- **No Skinning**: `ID3DXSkinMesh` and related functions are stubbed with `D3DXERR_SKINNINGNOTSUPPORTED` due to lack of programmable vertex processing.
- **Limited Texture Support**: `IDirect3DTexture8` and `ID3DXSprite` are stubbed until texture handling is implemented.
- **No Flow Control**: D3D8 flow control (e.g., `if`, `loop`) is not supported, as OpenGL ES 1.1 lacks equivalent functionality.
- **No File I/O**: Functions like `D3DXLoadMeshFromX` are stubbed, as file operations are out of scope for the shim.

## 7. Safety Checks for Agents

- **Do not** introduce allocations in rendering hot paths (e.g., `DrawIndexedPrimitive`); use pre-allocated buffers or `calloc` outside loops.
- **Do not** include C++ code or STL—the project is strictly C11.
- **Do not** add external dependencies (e.g., libpng, freetype) without a human-review gate.
- **Do not** modify `CMakeLists.txt` to change the build system (e.g., add dynamic linking) without explicit approval.
- **Run `ctest -V`** before proposing changes (tests to be added in future versions).
- **Verify** coordinate system conversions in all new matrix operations to ensure compatibility with OpenGL ES 1.1’s right-handed system.

## 8. Directory Structure
```
project_root/
├── include/
│   └── d3d8_to_gles.h   # Interface definitions and function prototypes
├── src/
│   └── d3d8_to_gles.c   # Core shim implementation
├── lib/
│   └── min-dx8-sdk/     # Reference DirectX 8 headers (not used for build)
├── CMakeLists.txt       # Build configuration
├── tests/               # (Future) Test cases
│   └── fixtures/
```

## 9. Adding New D3DX Shapes
- Implement shape creation functions (e.g., `D3DXCreateSphere`) by generating vertex/index buffers and populating `ID3DXMesh` with appropriate attribute tables.
- Ensure vertices are in D3D8’s left-handed coordinate system; rely on `d3d_to_gl_matrix` for rendering conversion.
- Example:
  ```c
  // Generate sphere vertices and indices
  VertexPN *vertices = calloc(num_vertices, sizeof(VertexPN));
  WORD *indices = calloc(num_faces * 3, sizeof(WORD));
  // Populate vertices/indices
  // Create vertex/index buffers and ID3DXMesh
  ```

## 10. Debugging and Logging
- Enable `D3D8_GLES_LOGGING` in `CMakeLists.txt` to activate debug logs.
- Use `d3d8_gles_log("Message: %s\n", details)` for tracing API calls or errors.
- Example:
  ```c
  d3d8_gles_log("SetTransform: state=%d, matrix=[%f, %f, %f, %f]\n",
                State, pMatrix->_11, pMatrix->_12, pMatrix->_13, pMatrix->_14);
  ```

---

Happy hacking! The maintainers welcome AI-generated contributions as long as they adhere to the constraints above, compile on both desktop GL stubs and real OpenGL ES 1.1 hardware, and respect the fixed-function pipeline limitations.