# Journal - CLIENT_BASE project

Append-only work log. Newest entries at the top.

---

## 2026-10-21 - L1 milestone closed

L1 live bridge is complete on `feature/client-base-role-improvements` (fork):

- `855871142` - secondary never-cancel + late window + Unity
- `5b81c9b5c` - wire-hash fix on cancel path (8/8 tests)
- Sims + code review: mesh wire-compatible (local TX policy only; no protobuf/role change)
- M2 primary TEXT opt-in and L2 module/poll remain next MVP work (not L1)

---

## 2026-10-21 - Post-change review: sims + L1 hash bug

### Sims re-run (unchanged conclusions)

- `sim_abstract` / `sim_variants` / `sim_platform_storage`: aggregate winner still **l1_secondary_only**; secondary airtime ~60 TX/100; L1-all ≈ ROUTER; L2 cursors / purge-on-ACK contrast holds; lean 256 B still fits 24h on nRF/ESP.

### Code review finding (fixed)

- **Bug:** `FloodingRouter::perhapsCancelDupe` runs in `shouldFilterReceived` **before** `perhapsDecode`. On that path `MeshPacket.channel` is still the **wire hash**, not a `ChannelIndex`.
- Using `isConfiguredSecondary(p->channel)` treated hash bytes as indices → for most hashes (`>= numChannels`) L1 secondary never-cancel was a **no-op**. Favorites path was unaffected.
- **Fix:** `Channels::isPacketOnConfiguredSecondary` (+ implement missing `getIndexByHash`); FloodingRouter uses it. Regression test `test_packet_on_secondary_resolves_wire_hash` - L1 suite **8/8 PASS**.

### L2 store notes (no code change)

- `nextSeq_` uint32 wrap after ~4e9 pushes would break seq arithmetic (not MVP-relevant).
- Store not yet wired to RX path - sims/L2 Unity validate the data structure only.

---

## 2026-10-21 - L2 store core (ring + cursors)

### Done

- `ClientBaseSecretaryStore` - 256 B lean records, shared ring, per-favorite cursors, TTL expire, capacity drop; **pull advances cursor only** (no purge-on-ACK)
- Unity `test_client_base_l2_store` - **7/7 PASSED** (multi-reader, TTL, cap, ring wrap)
- Not yet: module wiring, protobuf poll, LoRa unicast, flash backend

### Next

1. Wire store into a CLIENT_BASE-only module (ingest secondary TEXT; ACL = favorites)
2. Poll/unicast path + `history_return_max`
3. M2 primary TEXT opt-in (independent)

---

## 2026-10-21 - MVP L1 landed (code + Unity)

### Done

- Implemented **L1 secondary never-cancel / late window** for `CLIENT_BASE`:
  - `src/mesh/ClientBaseRelay.h` - pure policy helpers (`clientBaseAllowsCancelingDupe`, `clientBaseUsesLateRebroadcastWindow`)
  - `Channels::isConfiguredSecondary` - SECONDARY + `has_settings`
  - `FloodingRouter::roleAllowsCancelingDupe` / `perhapsCancelDupe` - CLIENT_BASE uses helpers; favorites still protected; primary stays CLIENT-like cancel
- Added suite `test/test_client_base_l1/` (7 Unity cases): channel classification + policy matrix + Channels→policy integration
- Native run on this host: **7/7 PASSED** (`pio test -e coverage -f test_client_base_l1`) after staging host libs under `.local-deps/` (not committed)

### Notes / caveats

- Host lacked system native deps (`yaml-cpp`, `libuv`, `gpiod`, `jsoncpp`, `libi2c`, OpenSSL). Workaround: extract debs into `.local-deps/` + `PLATFORMIO_BUILD_FLAGS` / `LD_LIBRARY_PATH`. Prefer Dockerfile.test / `apt install` for CI-like hosts.
- Dropped a NodeDB favorite-flag integration case that hung under the coverage runner (SIGHUP before `UNITY_END`); favorite×primary policy remains covered by the pure helper test.
- **M2** (opt-in primary TEXT) not in this commit - still default-off, unimplemented config key.
- **L2** store core landed in a follow-up commit (see entry above).

### Next (at L1 land time; mostly done)

1. Push L1 - done (`855871142`)
2. L2 store - done; module/poll next
3. Optional: wire M2 config when protobuf key exists / agreed
4. `gh auth login` → GitHub Project board

---

## 2026-10-21 (earlier) - Docs/policy lock + branch bootstrap

- Analysis + sims locked L1 secondary-only default; L2 multi-cursor; L3 deferred
- Project plane under `.cursor/client-base-project/`; rules/skills; first docs commit on fork
