#include "AssimpIO.h"

#include "CTransform.h"
#include "rig/create.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cstdio>

namespace rig {
namespace assimp {
namespace {

glm::vec4 diffuseColor(const aiMaterial* mat) {
	if (!mat) {
		return {1.f, 1.f, 1.f, 1.f};
	}
	aiColor4D c(1.f, 1.f, 1.f, 1.f);
	if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, c) != AI_SUCCESS) {
		return {1.f, 1.f, 1.f, 1.f};
	}
	return {c.r, c.g, c.b, c.a};
}

} // namespace

IoResult load(const std::string& path, rigkit::ecs::CMesh& outMesh) {
	IoResult result;
	outMesh = {};
	outMesh.mode = rigkit::ecs::CMesh::Mode::Triangles;

	Assimp::Importer importer;
	const unsigned flags = aiProcess_Triangulate | aiProcess_PreTransformVertices |
						   aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality |
						   aiProcess_GenSmoothNormals;
	const aiScene* scene = importer.ReadFile(path, flags);
	if (!scene || !scene->mRootNode ||
		(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !scene->HasMeshes()) {
		result.error = importer.GetErrorString();
		if (result.error.empty()) {
			result.error = "Assimp failed to load mesh";
		}
		return result;
	}

	bool anyUv = false;
	bool anyColor = false;
	unsigned usedMeshes = 0;
	unsigned skippedMeshes = 0;

	for (unsigned mi = 0; mi < scene->mNumMeshes; ++mi) {
		const aiMesh* mesh = scene->mMeshes[mi];
		if (!mesh || (mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0) {
			++skippedMeshes;
			continue;
		}

		const aiMaterial* mat = nullptr;
		if (scene->HasMaterials() && mesh->mMaterialIndex < scene->mNumMaterials) {
			mat = scene->mMaterials[mesh->mMaterialIndex];
		}
		const glm::vec4 faceCol = diffuseColor(mat);
		aiColor4D probe(1.f, 1.f, 1.f, 1.f);
		if (mat && mat->Get(AI_MATKEY_COLOR_DIFFUSE, probe) == AI_SUCCESS) {
			anyColor = true;
		}

		const bool hasUv = mesh->HasTextureCoords(0);
		unsigned facesAdded = 0;

		for (unsigned fi = 0; fi < mesh->mNumFaces; ++fi) {
			const aiFace& face = mesh->mFaces[fi];
			if (face.mNumIndices != 3) {
				continue;
			}
			outMesh.faceColors.push_back(faceCol);
			for (unsigned v = 0; v < 3; ++v) {
				const unsigned idx = face.mIndices[v];
				if (idx >= mesh->mNumVertices) {
					continue;
				}
				const aiVector3D& p = mesh->mVertices[idx];
				outMesh.positions.push_back({p.x, p.y, p.z});
				if (hasUv) {
					const aiVector3D& uv = mesh->mTextureCoords[0][idx];
					outMesh.texcoords.push_back({uv.x, uv.y});
					anyUv = true;
				} else {
					outMesh.texcoords.push_back({0.f, 0.f});
				}
			}
			++facesAdded;
		}

		if (facesAdded > 0) {
			++usedMeshes;
		} else {
			++skippedMeshes;
		}
	}

	if (!anyUv) {
		outMesh.texcoords.clear();
	}
	if (!anyColor) {
		outMesh.faceColors.clear();
	}

	if (outMesh.positions.empty()) {
		result.error = "Assimp scene contained no triangle faces";
		return result;
	}

	result.ok = true;
	if (skippedMeshes > 0) {
		char buf[128];
		std::snprintf(buf, sizeof(buf), "used %u mesh(es), skipped %u non-triangle mesh(es)",
					  usedMeshes, skippedMeshes);
		result.warning = buf;
	}
	return result;
}

entt::entity makeMeshFromFile(rigkit::MEcs& ecs, const std::string& path,
							  const rigkit::ecs::CDrawStyle& style, const std::string& name,
							  IoResult* outResult) {
	rigkit::ecs::CMesh mesh;
	IoResult loaded = load(path, mesh);
	if (outResult) {
		*outResult = loaded;
	}
	if (!loaded.ok) {
		return entt::null;
	}

	rigkit::ecs::CDrawStyle draw = style;
	if (!draw.hasFill && !draw.hasStroke) {
		draw = fill(1.f, 1.f, 1.f);
	}

	auto e = makeMesh(ecs, std::move(mesh.positions), std::move(mesh.indices), mesh.mode, draw,
					  name);
	auto& stored = ecs.getComponent<rigkit::ecs::CMesh>(e);
	stored.faceColors = std::move(mesh.faceColors);
	stored.texcoords = std::move(mesh.texcoords);
	return e;
}

} // namespace assimp
} // namespace rig
