#pragma once

#include "core/pack/IPack.h"

namespace rigkit {

/**
 * @brief Optional code pack — Assimp multi-format load into `CMesh`.
 * @details Leaf pack: no other RigKit pack depends on this. Apps opt in via `app.json`.
 * Prefer **rigObj** for Wavefront-only / Pi-default paths.
 */
class rigAssimp : public IPack {
  public:
	rigAssimp();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
