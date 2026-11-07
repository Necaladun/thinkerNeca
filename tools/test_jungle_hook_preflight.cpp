// Native host-only REND-001 safety test. No terranx.exe or Windows memory.
#include "../src/jungle_hook_preflight.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
constexpr unsigned char kOriginal[6] = {0x81, 0xE2, 0x04, 0, 0, 0x80};
constexpr unsigned char kRedirect[6] = {0xE9, 0, 0, 0, 0, 0x90};
constexpr int kAddresses[3] = {0x463CC5, 0x463D93, 0x464E78};

struct Site {
    int addr;
    unsigned char bytes[6];
};

void run_case(int changed_site, int changed_byte)
{
    Site sites[3] = {
        {kAddresses[0], {}},
        {kAddresses[1], {}},
        {kAddresses[2], {}},
    };
    for (int i = 0; i < 3; ++i) {
        std::memcpy(sites[i].bytes, kOriginal, sizeof(kOriginal));
    }
    if (changed_site >= 0) {
        sites[changed_site].bytes[changed_byte] ^= 0x80u;
    }

    // Snapshot the entire simulated instruction buffer, so rejecting any
    // preimage mismatch must leave *all three sites* byte-for-byte intact.
    Site before[3];
    std::memcpy(before, sites, sizeof(sites));
    int read_count = 0;
    int apply_count = 0;

    const bool installed = jungle_hook_preflight::validate_then_apply(
        sites,
        [&](const Site& site) -> bool {
            ++read_count;
            return std::memcmp(site.bytes, kOriginal, sizeof(kOriginal)) == 0;
        },
        [&](const Site& site) {
            // The real installer uses write_bytes followed by write_jump.
            // Count each site's single commit operation here.
            ++apply_count;
            Site& mutable_site = const_cast<Site&>(site);
            std::memcpy(mutable_site.bytes, kRedirect, sizeof(kRedirect));
        });

    if (changed_site < 0) {
        assert(installed);
        assert(read_count == 3);
        assert(apply_count == 3);
        for (const Site& site : sites) {
            assert(std::memcmp(site.bytes, kRedirect, sizeof(kRedirect)) == 0);
        }
    } else {
        assert(!installed);
        assert(read_count == changed_site + 1);
        assert(apply_count == 0);
        assert(std::memcmp(sites, before, sizeof(sites)) == 0);
    }
}
} // namespace

int main()
{
    run_case(-1, -1); // All valid: verify 3, commit 3.
    for (int changed_site = 0; changed_site < 3; ++changed_site) {
        for (int changed_byte = 0; changed_byte < 6; ++changed_byte) {
            run_case(changed_site, changed_byte);
        }
    }
    std::puts("REND-001: 1 success + 18 single-byte mismatch cases passed; "
              "all failures wrote zero bytes");
    return 0;
}
