# rigAssimp

![preview](examples/load/img/preview.png)

Optional **Assimp** multi-format load into [rigComponent](https://github.com/rigkid/rigComponent) `CMesh`. Code / IO only - no systems.

**Leaf pack.** Nothing in RigKit core or other packs depends on this. Apps that want FBX / glTF / COLLADA / STL / PLY (and more with a system Assimp) opt in via `app.json`. Wavefront-only and Pi-default paths stay on **rigObj**.


## API

```cpp
#include "AssimpIO.h"

rigkit::ecs::CMesh mesh;
auto r = rig::assimp::load("model.fbx", mesh);
auto e = rig::assimp::makeMeshFromFile(ecs, "model.gltf", rig::fill(1,1,1), "thing");
if (rig::assimp::isSupportedPath(path)) { /* drop / open */ }
```

Triangulates and bakes node transforms. Fills `texcoords` when present. Diffuse materials become `faceColors` when set. Skipped non-triangle meshes are reported in `IoResult::warning`. `supportedExtensions()` / `isSupportedPath()` report what this Assimp build can import - the load example opens a drop of any of those.

Default FetchContent importers: OBJ, FBX, glTF, COLLADA, STL, PLY. Prefer a system / vcpkg Assimp when present (`find_package(assimp CONFIG)` / `libassimp-dev`).

## Opt in

```json
{
  "name": "rigAssimp",
  "url": "https://github.com/rigkid/rigAssimp.git",
  "ref": "main"
}
```

```cpp
packs->registerPack(std::make_shared<rigkit::rigAssimp>());
```


## Pi / rebuild

Assimp is heavy. Do not add this pack to install apps that only need OBJ. Compiling Assimp on arm64 is a coffee-break risk - keep it out of the default Kit graph. CI uses `libassimp-dev` so the example does not FetchContent-compile Assimp.

[API/docs](https://rigkid.github.io/rigAssimp/)
