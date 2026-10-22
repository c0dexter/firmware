# Journal - CLIENT_BASE project

Append-only work log. Newest entries at the top.

---

## 2026-10-22 - Dump wire Unity + encode shrink-to-fit

- Extracted `ClientBaseSecretaryDump.h`; module calls `clientBaseSecretaryPrepareDumpPacket`.
- Unity `test_client_base_l2_dump` 3/3: ROUTER_TEXT not plain TEXT; 0/0+bitfield VALID; large lean payload shrinks until SF fits Data.payload.
- Pushed dump fix `babe610bf` to fork. Queue kept: #12180, L2-P4 nRF, M2 proto, UC6 field.

## 2026-10-22 - L2 dump TEXT E2E PASS (Xiao 3rd node)

- Desk: V4 roof + T114 home + Xiao nRF peer `!2cdd1088`.
- Home RX of forged dump failed twice: (1) channel TEXT DM → `Rejecting legacy DM`; (2) after SF wrap, `post-decode pre-hop drop` on 0/0 without `has_bitfield`.
- Fix on roof: dump as `ROUTER_TEXT_BROADCAST` + `has_bitfield=true` + fresh PacketId / `original_id`.
- **PASS:** home phone got `STORE_FORWARD ROUTER_TEXT_BROADCAST` fr=peer to=home with seed text. Uncommitted until operator asks.

## 2026-10-22 - L1.5 FreeHopBumpStart proven on desk

- Enabled `security.debug_log_api_enabled` on V4; subscribe `meshtastic.log.line`.
- Home favorite TX broadcast (ch0/ch1): roof logs `CLIENT_BASE L1.5 free hop from favorite: hop_start=4 hop_limit=3` after RX with `hopStart==hopLimit==3`.
- Push: overnight L2 commits on `fork` (`9a7a93669` / `ca9b956d4`).
- RF3 roof→home secondary retest **5/5 PASS** (earlier harness FAIL was listen race).

---

## 2026-10-22 - Overnight: L2 dump enable bug + fake NodeDB

- **Bugfix:** ctor `disable()` left `Thread::enabled=false`; `Started` only `setIntervalFromNow` → dump never ran. Fix: `enabled=true` before arming interval.
- **Bugfix:** begin/dump counted requester’s own TEXTs; desk 2-node looked like silent dump. Fix: `availableCountNotFrom` + skip `from==dest` in `sendNextPayload` (S&F-like).
- Desk: after flash, `ROUTER_HISTORY` PASS; fat NodeDB 45→82 via `fake_nodedb_stress.py`, rebootCount stable under fill (flashes bumped count later).
- L1.5 FreeHop still not CDC-proven. Summary: `MORNING_SUMMARY.md`.

---

## 2026-10-22 - Desk HW smoke (V4 + T114 clean)

- T114 wiped + clean UF2 `heltec-mesh-node-t114` → USB CDC restored; LongFast both nodes.
- Harness `scripts/desk_smoke.py`: S0/S1/S2/RF1/L15a/L2c/MEM1 PASS; L2 poll dump WARN (later fixed overnight); RF reverse FAIL/race.
- L1.5: favorite direct packets show `hopStart==hopLimit` at roof (precondition); FreeHop log not captured on CDC.
- Details: `HW_TEST_RESULTS.md`.

---

## 2026-10-21 - L1.5 free-hop (#8366)

- `ClientBaseHop.h` + wire in `NextHopRouter::perhapsRebroadcast`.
- FreeHopBumpStart: direct RF from favorite → keep hop_limit, hop_start++ (honest travel; next CB won’t free-hop again).
- DeliverAtZero: hop_limit already 0 to/from favorite → last-meter TX without inventing public budget.
- mayRelay includes both L1.5 outcomes (incl. free-hop when originator used hop_limit=0).
- Unity `test_client_base_l15_hop` **8/8 PASSED** (needs `configuration.h` for C-linkage `setup`/`loop`).
- Next: P5 hardware smoke (ESP32 + nRF available).

---

## 2026-10-21 - M2 primary elevate opt-in (runtime)

- Logic: `clientBaseShouldElevatePrimary` - TEXT/compressed when decoded; encrypted primary elevates when opt-in (cancel path can’t see portnum).
- Runtime flag `clientBaseSetElevatePrimaryOptIn` default **false** (UC2); protobuf persistence deferred.
- Channels: `isConfiguredPrimary` / `isPacketOnConfiguredPrimary`; FloodingRouter wired.
- Unity L1 extended with M2 cases.

---

## 2026-10-21 - L2-P3 caps / observability / docs

- D2 interval constant `CLIENT_BASE_SECRETARY_PACKET_INTERVAL_MS`; module stats enqueued/pulled/busy/rejected (no INFO on ingest hot path).
- `schemas.md` S&F poll sequence; UC5/UC6 wire notes; L2_PLAN DoD checked (stretch: hardware smoke).
- Full `test_client_base_l2*` sweep.

---

## 2026-10-21 - L2-P2 poll session + module dump path

- `ClientBaseSecretaryPoll.h` - ACL/busy/history_return_max/abort; Unity `test_client_base_l2_poll` **6/6**.
- Module: STORE_FORWARD CLIENT_HISTORY (favorite only) → ROUTER_HISTORY / BUSY; spaced TEXT unicast hop_limit=0 on stored secondary.
- Pushed prior P0/P1 commits to fork; tip includes this work after commit.

---

## 2026-10-21 - L2-P1 ingest

- `ClientBaseSecretaryIngest.h` + module `wantPacket` TEXT; secondary-only, skip own TX; CONTINUE (don’t steal phone path).
- Unity `test_client_base_l2_ingest` **5/5 PASSED**.
- Next: L2-P2 poll (D1=A StoreForward RR).

---

## 2026-10-21 - L2-P0 skeleton + D1=A

- Locked **D1=A** (reuse StoreForward RR for poll later; never co-enable classic S&F server).
- Added `ClientBaseSecretaryPolicy.h` (D2 caps), `ClientBaseSecretaryModule` (store only, wantPacket=false), Modules.cpp gate.
- Unity `test_client_base_l2_policy` **4/4 PASSED**; L1 + L2 store still green.
- Next: L2-P1 ingest.

---

## 2026-10-21 - Backlog: L1.5 free-hop for favorites (#8366)

- Operator: important that **dom→dach** via `CLIENT_BASE` not burn `hop_limit` when household is favorited.
- Today’s gap documented; item added to KANBAN / TODO / KB / UPSTREAM_WATCH P2 / ALIGNMENT §D / STATUS.
- Schedule: after L2 module (or with M2); design gate on hop-preserve loops first.

---

## 2026-10-21 - Alignment memo (rejection risk)

- Wrote **ALIGNMENT_WITH_UPSTREAM.md**: same problem as #12156/#12180; our secondary-scoped default is _more_ maintainer-aligned on airtime than all-broadcast fill-in.
- L2 backed by #10704 narrative; don’t fight #12180 - rebase/complement.
- Verdict: good direction; main process risk is merge order with #12180, not “wrong idea”.

---

## 2026-10-21 - Upstream 2.8/2.9 CLIENT_BASE overlap deep-dive

- **P0:** [#12156](https://github.com/meshtastic/firmware/issues/12156) + open PR **[#12180](https://github.com/meshtastic/firmware/pull/12180)** (→ develop / milestoned ~2.8.3): live fill-in for **all broadcasts** when a **direct favorite** exists - same UC1 pain, broader airtime than our secondary-only L1.
- **L2:** [#10704](https://github.com/meshtastic/firmware/issues/10704) (stale) asked for roof pull / 0-hop S&F - validates secretary direction.
- **2.9 label:** only #9942 (roles) + #8347 (gate ROUTER via admin). CLIENT_BASE likely stays user-settable.
- **Action on resume:** rebase develop; reconcile L1 vs #12180 before more cancel-path work. Full matrix: `UPSTREAM_WATCH.md`.

## 2026-10-21 - Upstream version + 3.0 watch (notes only)

- Baseline: `git describe` ≈ **v2.8.1…**; tip ≈ current **`origin/develop`**. Not on stale master.
- 3.0: label/holding area (packet structure #10042, roles #9942, …). No release date.
- Notes expanded in `UPSTREAM_WATCH.md`.

---

## 2026-10-21 - L2 plan ready + pause

Operator pause continues; execution plan written so coding can start cold.

- **[L2_PLAN.md](L2_PLAN.md)** - D1/D2 decisions, phases P0-P5, files, tests, DoD, session order.
- **[STATUS.md](STATUS.md)** - snapshot updated to point at the plan.
- Resume: lock D1 → P0 skeleton → P1 ingest → P2 poll → P3 caps/docs.

## 2026-10-21 - Pause before L2 module

Operator pause: L2 module/poll deferred until next free session.

**Snapshot:** see **[STATUS.md](STATUS.md)**.

- L1 closed + pushed (incl. wire-hash fix); review says mesh-compatible (local policy only).
- L2 store core on tree; **not** wired to MeshModule / protobuf / unicast yet.
- Resume at: L2 ingest + poll protocol decision → module + Unity.

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
