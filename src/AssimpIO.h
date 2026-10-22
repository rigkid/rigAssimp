#pragma once

/**
 * @file
 * @brief Import an Assimp-supported mesh file as a triangle `CMesh` or as a new mesh entity.
 * @details Which formats work depends on the Assimp build. Leaf pack - nothing else depends on
 * it, so an app opts in when it wants the extra formats.
 */

#include <string>
#include <vector>

#include "CDrawStyle.h"
#include "CMesh.h"
#include "CRenderMaterial.h"
#include "ecs/MEcs.h"

namespace rig {
namespace assimp {

struct IoResult {
	bool ok = false;
	std::string error;
	std::string warning;
};

/**
 * @brief Factors read from the file's first shaded material.
 * @details `present` is false when the file has no color, metal, roughness, or
 * texture key. `albedoPath` is empty when there is no external image (`*N`
 * embedded textures are skipped).
 */
struct MaterialFile {
	bool present = false;
	rigkit::ecs::CRenderMaterial material;
	std::string albedoPath;
};

/**
 * @brief Load a mesh file via Assimp into a triangle `CMesh`.
 * @details Triangulates and bakes node transforms (`PreTransformVertices`).
 * Fills `texcoords` when present. Diffuse becomes `faceColors` when set.
 * When @p outMaterial is set, the first glTF / Assimp PBR or diffuse material is
 * copied there (base color, metallic, roughness, emissive, external albedo path).
 * Formats depend on the Assimp build (OBJ / FBX / glTF / COLLADA / STL / PLY by default).
 * @param path File path.
 */
IoResult load(const std::string& path, rigkit::ecs::CMesh& outMesh, MaterialFile* outMaterial = nullptr);

/**
 * @brief Extensions this Assimp build can import, lowercase, no dot (`stl`, `obj`).
 * @details Comes from Assimp (`GetExtensionList`), so a system Assimp with more
 * importers reports more than the FetchContent default (OBJ / FBX / glTF /
 * COLLADA / STL / PLY). CAD STEP (`.stp` / `.step`) is not a real Assimp path.
 */
std::vector<std::string> supportedExtensions();

/** @brief True when this Assimp build lists the path's extension. */
bool isSupportedPath(const std::string& path);

/**
 * @brief Load via Assimp and spawn `CTransform` + `CMesh` + `CDrawStyle`.
 * @details Adds `CRenderMaterial` when the file has shading factors. An external
 * albedo image becomes a `CAssetRef` entity named `<name>-albedo`.
 * @return Entity id, or `entt::null` on failure (see `outResult`).
 */
entt::entity makeMeshFromFile(rigkit::MEcs& ecs, const std::string& path,
							  const rigkit::ecs::CDrawStyle& style = {},
							  const std::string& name = {}, IoResult* outResult = nullptr);

} // namespace assimp
} // namespace rig
