# CLI Utilities

This directory contains command-line helpers for the project.

## `egl_config_cli`

`egl_config_cli` probes the system's EGL implementation and prints the attributes of a matching configuration. It accepts options that mirror the fields of `D3DPRESENT_PARAMETERS` so you can easily translate a DirectX 8 game's settings to OpenGL ES 1.1.

### Building

```bash
cmake -S . -B build
cmake --build build
```

### Choosing an EGL config

Run the tool with flags describing the swap chain you need. For example:

```bash
./build/egl_config_cli --window --depth --width 800 --height 600
```

The program prints the color, depth, stencil, and multisample values of the chosen configuration.

### When porting without this shim

If you are rewriting a D3D8 game to call OpenGL ES directly, you can use `egl_config_cli` to discover which configurations are available on the target hardware. Match these values when creating your EGL context so that your rendering code works similarly to its original D3D8 setup.

In this use case `egl_config_cli` acts as a small shim: it replicates the config selection logic of `d3d8_to_gles` without requiring you to integrate the full library. Use the reported values to fill in `eglChooseConfig` attributes or to verify your desired surface is supported.
