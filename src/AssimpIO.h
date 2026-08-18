#pragma once

#include <string>

#include "CDrawStyle.h"
#include "CMesh.h"
#include "ecs/MEcs.h"

namespace rig {
namespace assimp {

struct IoResult {
	bool ok = false;
	std::string error;
	std::string warning;
};

/**
 * @brief Load a mesh file via Assimp into a triangle `CMesh`.
 * @details Triangulates and bakes node transforms (`PreTransformVertices`).
 * Fills `texcoords` when present. Materials become `faceColors` (diffuse) when set.
 * Formats depend on the Assimp build (OBJ / FBX / glTF / COLLADA / STL / PLY by default).
 * @param path File path.
 */
IoResult load(const std::string& path, rigkit::ecs::CMesh& outMesh);

/**
 * @brief Load via Assimp and spawn `CTransform` + `CMesh` + `CDrawStyle`.
 * @return Entity id, or `entt::null` on failure (see `outResult`).
 */
entt::entity makeMeshFromFile(rigkit::MEcs& ecs, const std::string& path,
							  const rigkit::ecs::CDrawStyle& style = {},
							  const std::string& name = {}, IoResult* outResult = nullptr);

} // namespace assimp
} // namespace rig
