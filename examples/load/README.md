# rigAssimp - load

Hero for [rigAssimp](../..). Loads `data/prism.stl` and `data/cube.obj` through Assimp into `CMesh`, presents with rigRender3D. Drop any mesh this Assimp build imports (STL, OBJ, FBX, glTF, COLLADA, PLY by default) onto the window to replace the scene.

Orbit inspect: MMB or Alt+LMB tumble, Shift+MMB or Alt+MMB pan, Alt+RMB / wheel dolly, F frames.

```bash
cmake -S . -B build
cmake --build build --target load
```
