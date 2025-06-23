#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <d3d8_to_gles.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(const char *prog) {
  printf("Usage: %s [options]\n", prog);
  printf("Options:\n");
  printf(
      "  --window            Use window surface (default offscreen pbuffer)\n");
  printf("  --width  <w>        Backbuffer width (default 64)\n");
  printf("  --height <h>        Backbuffer height (default 64)\n");
  printf("  --depth             Enable depth/stencil (16/8 bits)\n");
  printf("  --samples <n>       Multisample count (default 0)\n");
  printf("  --help              Display this help and exit\n");
}

static EGLConfig choose_config(EGLDisplay display, D3DPRESENT_PARAMETERS *pp,
                               BOOL want_window) {
  EGLint attribs[] = {
      EGL_RED_SIZE,
      (pp->BackBufferFormat == D3DFMT_A8R8G8B8 ||
       pp->BackBufferFormat == D3DFMT_X8R8G8B8)
          ? 8
          : 5,
      EGL_GREEN_SIZE,
      (pp->BackBufferFormat == D3DFMT_A8R8G8B8 ||
       pp->BackBufferFormat == D3DFMT_X8R8G8B8)
          ? 8
          : 6,
      EGL_BLUE_SIZE,
      (pp->BackBufferFormat == D3DFMT_A8R8G8B8 ||
       pp->BackBufferFormat == D3DFMT_X8R8G8B8)
          ? 8
          : 5,
      EGL_ALPHA_SIZE,
      (pp->BackBufferFormat == D3DFMT_A8R8G8B8) ? 8 : 0,
      EGL_DEPTH_SIZE,
      pp->EnableAutoDepthStencil ? 16 : 0,
      EGL_STENCIL_SIZE,
      pp->EnableAutoDepthStencil ? 8 : 0,
      EGL_SURFACE_TYPE,
      want_window ? EGL_WINDOW_BIT : EGL_PBUFFER_BIT,
      EGL_RENDERABLE_TYPE,
      EGL_OPENGL_ES_BIT,
      EGL_SAMPLES,
      pp->MultiSampleType >= D3DMULTISAMPLE_2_SAMPLES ? pp->MultiSampleType : 0,
      EGL_NONE};

  EGLConfig config;
  EGLint num;
  if (!eglChooseConfig(display, attribs, &config, 1, &num) || num == 0)
    return NULL;
  return config;
}

int main(int argc, char **argv) {
  BOOL want_window = FALSE;
  D3DPRESENT_PARAMETERS pp = {0};
  pp.BackBufferWidth = 64;
  pp.BackBufferHeight = 64;
  pp.BackBufferFormat = D3DFMT_X8R8G8B8;
  pp.BackBufferCount = 1;
  pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  pp.Windowed = TRUE;
  pp.EnableAutoDepthStencil = FALSE;
  pp.MultiSampleType = D3DMULTISAMPLE_NONE;
  pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--window") == 0) {
      want_window = TRUE;
    } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
      pp.BackBufferWidth = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
      pp.BackBufferHeight = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--depth") == 0) {
      pp.EnableAutoDepthStencil = TRUE;
    } else if (strcmp(argv[i], "--samples") == 0 && i + 1 < argc) {
      pp.MultiSampleType = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--help") == 0) {
      print_help(argv[0]);
      return 0;
    } else {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      print_help(argv[0]);
      return 1;
    }
  }

  EGLDisplay display = EGL_NO_DISPLAY;
  if (getenv("DISPLAY")) {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(display, NULL, NULL)) {
      display = eglGetPlatformDisplay(
          EGL_PLATFORM_SURFACELESS_MESA,
          (void *)(intptr_t)EGL_DEFAULT_DISPLAY,
          NULL);
    }
  } else {
    display = eglGetPlatformDisplay(
        EGL_PLATFORM_SURFACELESS_MESA,
        (void *)(intptr_t)EGL_DEFAULT_DISPLAY,
        NULL);
  }
  if (!eglInitialize(display, NULL, NULL)) {
    fprintf(stderr, "Failed to initialize EGL\n");
    return 1;
  }

  EGLConfig cfg = choose_config(display, &pp, want_window);
  if (!cfg) {
    fprintf(stderr, "Failed to choose EGL config\n");
    eglTerminate(display);
    return 1;
  }

  printf("EGL config chosen with %s surface:\n",
         want_window ? "window" : "pbuffer");
  EGLint val;
  eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &val);
  printf("  red size: %d\n", val);
  eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &val);
  printf("  green size: %d\n", val);
  eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &val);
  printf("  blue size: %d\n", val);
  eglGetConfigAttrib(display, cfg, EGL_ALPHA_SIZE, &val);
  printf("  alpha size: %d\n", val);
  eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &val);
  printf("  depth size: %d\n", val);
  eglGetConfigAttrib(display, cfg, EGL_STENCIL_SIZE, &val);
  printf("  stencil size: %d\n", val);
  eglGetConfigAttrib(display, cfg, EGL_SAMPLES, &val);
  printf("  samples: %d\n", val);

  eglTerminate(display);
  return 0;
}
