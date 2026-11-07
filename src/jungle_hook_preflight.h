#pragma once

// REND-001: common two-phase installation control flow. This template
// has no direct access to the game executable. Real x86 hook callbacks
// verify native preimage bytes and apply the jump; the CI harness passes
// synthetic byte buffers and records writes to prove fail-closed logic.
#include <cstddef>

namespace jungle_hook_preflight {

template <typename Site, std::size_t N, typename Verify, typename Apply>
bool validate_then_apply(const Site (&sites)[N], Verify verify, Apply apply)
{
    static_assert(N == 3, "Native Jungle hook requires exactly three sites");
    for (std::size_t i = 0; i < N; ++i) {
        if (!verify(sites[i])) {
            return false;
        }
    }
    // No patch operation can run until all three checks succeeded.
    for (std::size_t i = 0; i < N; ++i) {
        apply(sites[i]);
    }
    return true;
}

} // namespace jungle_hook_preflight
