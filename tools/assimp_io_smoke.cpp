#include "AssimpIO.h"

#include <cstdio>
#include <cstdlib>

#ifndef RIGASSIMP_SAMPLE_STL
#define RIGASSIMP_SAMPLE_STL ""
#endif

int main(int argc, char** argv) {
	const char* path = nullptr;
	if (argc >= 2) {
		path = argv[1];
	} else if (RIGASSIMP_SAMPLE_STL[0] != '\0') {
		path = RIGASSIMP_SAMPLE_STL;
	} else {
		std::fprintf(stderr, "usage: assimp_io_smoke <mesh>\n");
		return 2;
	}

	if (!rig::assimp::isSupportedPath(path)) {
		std::fprintf(stderr, "unsupported extension (%s)\n", path);
		return 1;
	}

	rigkit::ecs::CMesh mesh;
	const auto r = rig::assimp::load(path, mesh);
	if (!r.ok) {
		std::fprintf(stderr, "load failed (%s): %s\n", path, r.error.c_str());
		return 1;
	}
	if (!r.warning.empty()) {
		std::fprintf(stderr, "warning: %s\n", r.warning.c_str());
	}
	const size_t faces =
		mesh.faceColors.empty() ? mesh.positions.size() / 3 : mesh.faceColors.size();
	std::printf("ok path=%s vertices=%zu texcoords=%zu faces=%zu\n", path, mesh.positions.size(),
				mesh.texcoords.size(), faces);
	return 0;
}
