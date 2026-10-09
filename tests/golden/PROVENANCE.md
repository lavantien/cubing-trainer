# visual golden provenance

generated 2026-10-10 on windows 11 10.0.26300, NVIDIA GeForce RTX 3080, OpenGL 3.3.0 NVIDIA 617.42, clang 23.1.3 targeting x86_64-w64-mingw32 over the winlibs UCRT sysroot, vendored raylib 6.0. rendering path: hidden GLFW window (SetConfigFlags(FLAG_WINDOW_HIDDEN) before InitWindow) with OpenGL 3.3, offscreen RenderTexture2D 960x540, LoadImageFromTexture plus ImageFlipVertical readback, raylib ExportImage PNG encoder. the rlsw software renderer was evaluated and rejected for this purpose, see playground/headless_spike/REPORT.md.

scene parameters shared by all goldens: camera position (4.5, 4.5, 4.5), target origin, up (0, 1, 0), fovy 45, perspective, ct_color_scheme_default (U white, L orange, F green, R red, B blue, D yellow), ClearBackground RAYWHITE, CT_RENDERER_MOVE_MILLIS 350.

hashes are SHA-256 computed with python 3.14.7 hashlib (certutil -hashfile <file> SHA256 gives the same value).

| file | scene | n | alg | mid state | sha256 |
| --- | --- | --- | --- | --- | --- |
| golden_solved_3.png | solved | 3 | none | none | 70d021874787b2d608bd9f3be17e7ce66632d28ed856a10a258967cf0c3fba12 |
| golden_solved_7.png | solved | 7 | none | none | 6954130954b0d0183bf4edad99c1d3c6d7a52e309e3d4195a7424ea93b2de5a |
| golden_scramble_3.png | scramble | 3 | R U R' U R U2 R' | none | 964f90cdcbe5ebf22c7b33ded7b6fec129db669a73699e1382c3889fadf5e5622 |
| golden_mid_3.png | mid | 3 | R U R' U R U2 R' | move R, progress 0.5 | 8d3f7c87140b67660fee7adbbcec342e78cac7bcdd5eb216c67e78f86c94d5ca |

generation commands, run from the repo root after building build/bin/render_scene.exe (build line in the commit message and in playground/headless_spike/REPORT.md):

```
build/bin/render_scene.exe solved 3 tests/golden/golden_solved_3.png
build/bin/render_scene.exe solved 7 tests/golden/golden_solved_7.png
build/bin/render_scene.exe scramble 3 "R U R' U R U2 R'" tests/golden/golden_scramble_3.png
build/bin/render_scene.exe mid 3 "R U R' U R U2 R'" tests/golden/golden_mid_3.png
```

verification recorded at generation time: each scene regenerated 3 times in separate processes produced byte identical SHA-256 values against the committed golden, python -I tools/visual_check.py --selfcheck passed, all 4 golden versus fresh comparisons passed twice, a single sticker recolor (40x40 green to blue patch on golden_scramble_3) failed comparison, and a one token alg edit (trailing prime dropped) failed comparison.

machine dependence: the goldens encode GPU rasterization output. a different GPU, driver, or window-system path requires regenerating them with the same commands and updating this file. byte determinism was verified on the generating machine only.
