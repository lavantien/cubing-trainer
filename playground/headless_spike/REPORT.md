# headless render spike record

date: 2026-10-10, machine: windows 11 10.0.26300, GPU: NVIDIA GeForce RTX 3080, driver 617.42 (OpenGL 3.3.0), toolchain: clang 23.1.3 targeting x86_64-w64-mingw32 over the winlibs UCRT sysroot, raylib 6.0 vendored in third_party/raylib.

## question

can the cubing-trainer render scenes to PNG without a visible window, using either the raylib 6 rlsw software renderer or a hidden GLFW window with RenderTexture2D readback.

## path A: rlsw software renderer, PLATFORM_MEMORY windowless

defines: -DPLATFORM_MEMORY -DGRAPHICS_API_OPENGL_SOFTWARE -DSW_FRAMEBUFFER_OUTPUT_BGRA=false -D_GNU_SOURCE -fno-strict-aliasing -Wno-missing-braces, link -lwinmm only (no opengl32, no gdi32, no rglfw.c needed). sources: rcore.c rshapes.c rtextures.c rmodels.c rtext.c plus src/engine and src/render.

findings:

1. windowless init works. InitWindow creates no window, no display is required. InitPlatform allocates a plain RGBA memory framebuffer and rlglInit calls swInit. artifacts: build/headless_spike_orientation.png, build/headless_spike_cube.png, build/headless_spike_builtins.png.
2. compile quirk: rlsw.h does "#include __FILE__" to re-include itself for the implementation section. when raylib sources are passed as third_party/raylib/src/rcore.c, clang builds a mixed-separator __FILE__ spelling (third_party/raylib/src\external/rlsw.h) that fails to resolve. adding -I. to the compile fixes it because the mixed path resolves from the repo root.
3. color channel swap: with the rlsw default SW_FRAMEBUFFER_OUTPUT_BGRA=true, glReadPixels(GL_RGBA) returns B,G,R,A byte order. raylib RED came back (55,41,230). compiling with -DSW_FRAMEBUFFER_OUTPUT_BGRA=false returns true RGBA.
4. vertical flip: swReadPixels already returns top-down rows, then rlReadScreenPixels flips again, so LoadImageFromScreen yields a bottom-up image. one CPU-side ImageFlipVertical restores raylib screen convention. verified with a two color half frame: top half drawn RED reads (230,41,55) at image top, bottom half drawn BLUE reads (0,121,241).
5. 2D rasterization is exact: the two color frame contains exactly 2 distinct colors, the clear color probe is bit exact.
6. 3D rasterization is broken in this vendored rlsw 1.5 snapshot. raylib's own DrawCube renders shattered geometry: wrong triangle seams, trapezoid fragments, stray triangles (build/headless_spike_builtins.png). the ct renderer dynamic mesh is equally mangled (build/headless_spike_cube.png): faces bleed into each other in vertical stripes, silhouette is ragged. the same scene renders correctly under OpenGL 3.3 (build/render_smoke_end.png from the windowed smoke, and path B below).
7. verdict: rlsw is not viable as the headless backend for this milestone. the defect is inside third_party code which is out of scope to patch.

## path B: hidden GLFW window plus RenderTexture2D readback

defines: -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 -D_GNU_SOURCE -DUNICODE -fno-strict-aliasing -Wno-missing-braces, includes -Ithird_party/raylib/src/external/glfw/include, link -lopengl32 -lgdi32 -lwinmm. sources: rcore.c rglfw.c rshapes.c rtextures.c rmodels.c rtext.c plus src/engine and src/render.

findings:

1. SetConfigFlags(FLAG_WINDOW_HIDDEN) before InitWindow makes rcore_desktop_glfw.c set GLFW_VISIBLE to FALSE, the window is never shown. verified in the vendored source at third_party/raylib/src/platforms/rcore_desktop_glfw.c around the glfwWindowHint(GLFW_VISIBLE) call.
2. readback: LoadRenderTexture(w,h), BeginTextureMode and EndTextureMode around ClearBackground plus ct_renderer_draw, LoadImageFromTexture(rt.texture), ImageFlipVertical (texture origin is bottom-left), ExportImage. orientation probe: top half drawn RED reads exactly e62937ff at image top, bottom BLUE reads 0079f1ff.
3. the solved cube renders identical to the windowed GL33 smoke: white up, green front left, red front right on RAYWHITE. artifact: build/headless_hidden_cube.png.
4. determinism: in-process digests match, and exported PNG bytes are SHA-256 identical across 3 separate process runs (fresh GL context each time): 70D021874787B2D608BD9F3BE17E7CE66632D28ED856A10A258967CF0C3FBA12 for the cube frame. no MSAA is requested.
5. caveat: requires an interactive desktop session and the GPU stack, so goldens are machine specific. provenance records the GPU and driver. a display-less CI agent cannot run this path.

## verdict

headless backend = path B, hidden GLFW window plus RenderTexture2D readback, implemented in src/app/headless.c. the compile line is identical in defines to the normal app raylib build, so the Makefile can reuse the same RAYLIB_OBJ set for build-headless, no separate raylib compilation is needed.

## reproduction

see the exact clang invocations in the git history of this file's companion sources, playground/headless_spike/main.c (path A) and playground/headless_spike/hidden.c (path B), and tools/render_scene.c which supersedes both as the maintained entry point.