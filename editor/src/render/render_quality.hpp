// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <array>
#include <cmath>
#include <cstring>
#include <span>
#include <vector>
#include "render/renderer.hpp"

namespace roadmaker::editor {
// Owned contents, never pointer identity: actors mutate their matrices in place
// and different draw items can share a mesh with different instance spans.
class InstanceBufferCache {
public:
  bool update(std::span<const InstanceData> instances) {
    static_assert(sizeof(InstanceData) == 16 * sizeof(float));
    if (data_.size() == instances.size() &&
        (instances.empty() || std::memcmp(data_.data(), instances.data(),
                                         instances.size_bytes()) == 0)) return false;
    data_.assign(instances.begin(), instances.end());
    return true;
  }
private:
  std::vector<InstanceData> data_;
};

// Orthographic directional shadows only. Quantize the projection origin to
// whole texels, so camera translation does not slide the sampling grid over
// stationary surfaces. Never apply this to perspective street-light maps.
inline std::array<float,16> stabilize_shadow_projection(
    std::array<float,16> matrix, int resolution) {
  if (resolution <= 0) return matrix;
  const float half = static_cast<float>(resolution) * 0.5F;
  matrix[12] = std::round(matrix[12] * half) / half;
  matrix[13] = std::round(matrix[13] * half) / half;
  return matrix;
}
} // namespace roadmaker::editor
