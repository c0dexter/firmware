# Knowledge base - CLIENT_BASE project

_Last updated: 2026-10-21 (L1.5 free-hop shipped)_

Canonical long-form analysis: **[ANALYSIS_AND_SIMULATION.md](ANALYSIS_AND_SIMULATION.md)**.  
Work log: **[JOURNAL.md](JOURNAL.md)**. Execution plan: **[L2_PLAN.md](L2_PLAN.md)**.  
Upstream / 3.0: **[UPSTREAM_WATCH.md](UPSTREAM_WATCH.md)**.

## Decisions (accepted)

0. **L2 D1 poll protocol (LOCKED 2026-10-21): Option A** - reuse `STORE_FORWARD_APP` / `StoreAndForward` RR for poll when P2 lands. Gate: `role==CLIENT_BASE`, favorite ACL, **never** enable classic S&F server on the same node (`Modules.cpp` skip). Prefer A for MVP speed; B (new portnum) remains FVP/upstream polish if product boundary hurts. **D2 caps** coded in `ClientBaseSecretaryPolicy.h` (TTL 24h, max favorites 8, history_return_max 32, ring by MEM_CLASS ≤256).
1. **Keep role name `CLIENT_BASE`** - solid fix + secretary upgrade, not a new role.
2. **Root user pain:** favorites elevate only `from`/`to`; channel broadcasts from the mesh are not elevated → roof phone sees messages, home often does not (cancel-on-dupe + hidden terminal) → operators install ROUTER/ROUTER_LATE.
3. **L1 + L2 together:** live fix for in-range home; secretary for offline / missed window. L2 alone is insufficient.
4. **L1 default (LOCKED):** late never-cancel only on **secondary channels configured on the base**. Wins multi-world system score; lowest public TX among effective fixes.
5. **L1 opt-in (LOCKED):** elevate **primary TEXT** - default **off**. Runtime wired (`clientBaseSetElevatePrimaryOptIn`); protobuf persist deferred. Encrypted primary elevates when opt-in (cancel path can’t see portnum); decoded non-TEXT primary does not.
6. **L1 forbidden default (LOCKED):** never-cancel on all channels / stealth ROUTER_LATE.
7. **L2 (LOCKED):** TEXT-focused buffer on those secondaries; shared ring; **per-favorite cursor**; TTL/ring eviction; **never purge-on-first-ACK**.
8. **Platforms (LOCKED from `sim_platform_storage.py`):** LEAN 256 B records; nRF = flash+index (viable for 24h@4/h); ESP no-PSRAM RAM ring OK for 24h; ESP+PSRAM must cap for dump time; STM32WL = L2 excluded. Not a PSRAM-only global S&F clone.
9. **L3** peer sync after TTL: acknowledged; **not MVP**.
10. **Tracking:** this folder + Cursor todos; GitHub Project when `gh` re-authed; no root `docs/` in firmware.
11. **Quality bar:** docs here + unit tests + hot-path performance bounds mandatory.
12. **MVP vs FVP:** defined in `MVP_AND_FVP.md`. MVP = L1 secondary + L2 secretary/cursors/caps/tests. FVP = MVP + app UX + hardening + L3 (phone BT sync first, LoRa peer optional).
13. **L1 code (2026-10-21):** shipped on this branch - see Findings.

## Findings (code)

### Pre-L1 (problem)

- `FloodingRouter::roleAllowsCancelingDupe`: CLIENT_BASE cancelled unless `isFromOrToFavoritedNode`.
- Broadcast: `isFromOrToFavoritedNode` → only `isFavorite(from)`.
- Late window for favorite from/to did not help generic mesh→home broadcasts.
- `shouldRebroadcastEarlyLikeRouter`: only ROUTER.
- Auto-favorite suppressed on CLIENT_BASE (favorite overloaded).
- Existing S&F `lastRequest[dest]` cursors - pattern to mirror for L2 (separate product).

### L1 implementation (current)

- `Channels::isConfiguredSecondary(idx)` - `has_settings` && role `SECONDARY`.
- `Channels::isPacketOnConfiguredSecondary(p)` - **required** on cancel-on-dupe path: pre-decode `p.channel` is wire hash; post-decode it is index. (`getIndexByHash` now implemented.)
- `ClientBaseRelay.h` - cancel/late helpers + M2 `clientBaseShouldElevatePrimary` / runtime opt-in get/set.
- `FloodingRouter`: never-cancel + late when favorite **or** secondary **or** M2 elevate-primary; default opt-in off.
- `Channels`: `isPacketOnConfiguredPrimary` (hash-safe like secondary).
- Tests: `test/test_client_base_l1/` - 13/13 (wire-hash + M2 matrix).

### Post-change review (2026-10-21)

- Sims re-run: policy lock still holds (`l1_secondary_only` aggregate winner).
- Caught and fixed hash-vs-index bug on L1 cancel path (see JOURNAL).

### L2 store core (current)

- `ClientBaseSecretaryStore` - calloc ring + fixed cursor slots; lean 256 B records; `push` / `expire` / `ensureFavorite` / `pullNext` (cursor-only advance).
- Tests: `test/test_client_base_l2_store/` - 7/7 (multi-reader no purge, TTL, capacity, favorite cap).
- Still open: ingest (P1), poll RR (P2), flash spill for nRF, session `history_return_max` enforcement on air.
- **Execution plan:** `L2_PLAN.md` (D1 locked A; phases P0-P5).

### L2-P0 module skeleton (current)

- `ClientBaseSecretaryPolicy.h` - enable gate + D2 caps (Unity `test_client_base_l2_policy` 4/4).
- `ClientBaseSecretaryModule` - owns store; registered in `Modules.cpp` only for CLIENT_BASE, skipped if StoreForward enabled / STM32WL.

### L2-P1 ingest (current)

- `ClientBaseSecretaryIngest.h` - gates + fill lean record + `clientBaseSecretaryIngestText`.
- Module: ingest configured secondary TEXT only; skip `isFromUs`; always CONTINUE.
- Unity `test_client_base_l2_ingest` 5/5.

### L2-P2 poll (current)

- `ClientBaseSecretaryPoll.h` - begin/busy/empty/pull/abort; Unity `test_client_base_l2_poll` (incl. skip-own-from Empty).
- Module listens STORE_FORWARD `CLIENT_HISTORY` (favorite ACL silent reject); `ROUTER_BUSY` / `ROUTER_HISTORY`; dump as **STORE_FORWARD `ROUTER_TEXT_*`** unicast `hop_limit=0` on stored channel (D1=A); one dump at a time.
- **2026-10-22 desk bugs:** (1) after ctor `disable()`, must `enabled=true` before `setIntervalFromNow` or dump thread never runs; (2) exclude `from==requester` from available/dump (classic S&F); else 2-node self-seeds inflate header and forge from==to TEXT home ignores; (3) raw channel-PSK **TEXT** unicast is rejected as **legacy DM** (`Router::perhapsDecode`) - dump must wrap like classic S&F; (4) forged `from` skips `isFromUs` hop_start/bitfield init → receivers **post-decode pre-hop drop** on 0/0 without `has_bitfield` - set `decoded.has_bitfield=true` (and fresh PacketId; `original_id=rec.id`).
- **Desk E2E PASS (3-node):** peer Xiao `!2cdd1088` secondary TEXT → roof ingest → home poll → `ROUTER_TEXT_BROADCAST` from=peer to=home delivered to phone.
- **Dump helper:** `ClientBaseSecretaryDump.h` + Unity `test_client_base_l2_dump`. Shrink text until SF encodes into `DATA_PAYLOAD_LEN` (233); full 233-byte S&F text field does not fit framing (silent `pb_encode_to_bytes` → 0).
- **L2-P4.1:** ring ops behind `ClientBaseSecretaryRing` / `RamRing`; Store owns cursors only. Flash spill = P4.2 (`L2_P4_DESIGN.md`). Priority order: `QUEUE.md`.

### L2-P3 polish (current)

- Caps: `CLIENT_BASE_SECRETARY_PACKET_INTERVAL_MS`; module stats enqueued/pulled/busy/rejected (no INFO on ingest hot path).
- Docs: `schemas.md` S&F poll sequence; UC5/UC6; L2_PLAN DoD checked (stretch = hardware smoke).
- Next: **P5 hardware smoke** / M2 protobuf persist / L2-P4 nRF.

### L1.5 free-hop (current)

- `ClientBaseHop.h` - `clientBaseClassifyRelayHop`: FreeHopBumpStart (direct from favorite), DeliverAtZero (hop_limit==0 to/from favorite), else NormalDecrement.
- `NextHopRouter::perhapsRebroadcast` - mayRelay includes L1.5; FreeHop bumps `hop_start` (keep `hop_limit`); DeliverAtZero TX at 0.
- Loop safety: FreeHop only when `hopsAway==0`; after bump, next CLIENT_BASE sees hopsAway≥1 → NormalDecrement (no attic chain inventing budget).
- Unity `test_client_base_l15_hop`.
- Distinct from `shouldDecrementHopLimit` favorite-ROUTER preserve (still used on NormalDecrement path).
- **Desk 2026-10-22:** FreeHopBumpStart observed via LogRecord API (`hop_start` 3→4, `hop_limit` kept 3) on Heltec V4 CLIENT_BASE ← T114 favorite.

## Simulation summary (pointer)

| Artifact                  | Result highlight                                                                              |
| ------------------------- | --------------------------------------------------------------------------------------------- |
| `sim_abstract.py`         | Today ~63% home delivery vs L1-all ~86% but ~92% roof TX; secondary-scoped airtime ~60/100    |
| Private-only urban        | ~59% → ~86% with L1 secondary                                                                 |
| `sim_variants.py`         | Aggregate winner: **l1_secondary_only**; L1-all ≈ ROUTER scores                               |
| Multi-reader              | Cursors keep Bart fed; purge-on-ACK drops shared history                                      |
| Offline 6h                | L1 recovers 0; L1+L2 recovers buffer at local unicast cost                                    |
| `sim_platform_storage.py` | nRF/ESP/RP fit 24h TTL with lean records; dump ~4 min/96 msg; PSRAM needs caps; STM32WL no L2 |

Details and tables: `ANALYSIS_AND_SIMULATION.md` §9.

## Open questions

- Exact numeric caps to ship: `history_return_max`, TTL, records per MEM_CLASS (sim suggests 24h@4/h feasible; dump cap e.g. 32-64 msg/session).
- ~~Protobuf: extend StoreAndForward vs new private control messages~~ → **D1=A locked** (S&F RR reuse); revisit only if A proves too confusing.
- Exact protobuf config key name for primary TEXT opt-in (runtime flag exists; persist TBD).
- GitHub Project board ID after `gh auth login`.
- Upstream PRs: consider omitting `.cursor/` if reviewers prefer; keep on fork.
- **Upstream L1 overlap:** #12156 / PR **#12180** (all-broadcast fill-in if direct favorite) vs our secondary-only L1 - reconcile on resume (`UPSTREAM_WATCH.md`).
- **2.9 / 3.0:** #9942 roles; #10042 packet structure - watch, not L2 blockers.
- ~~**L1.5 free-hop (#8366)**~~ → shipped (see Findings). Field-validate on hardware with P5.

## Findings (hops / free-hop - L1.5)

- User ask (2026-10-21): household favorites get a “free” hop via roof `CLIENT_BASE` so **dom→dach** does not consume mesh hop budget (#8366).
- Pre-L1.5 gap: `shouldDecrementHopLimit` always −1 at hopsAway==0; preserve only favorite ROUTER/CLIENT_BASE peers - favorite **CLIENT** still burned a hop.
- Shipped heuristic: FreeHopBumpStart + DeliverAtZero in `ClientBaseHop.h` / `NextHopRouter` (see L1.5 findings above). Not public amplify: favorites ACL only.

## Upstream baseline (2026-10-21)

- Working on **2.8.1-line / `develop`**. Latest release: **2.8.1 Alpha**.
- Competing UC1 fix: open **PR #12180** (~2.8.3). Locked default stays secondary-scoped until we deliberately change policy after rebase/compare.
- **2.9** label thin (#9942, #8347). **3.0** = future major; no ship date.

## Rejected / deferred

- Mandatory visible `<thinking>` every reply (original mega-prompt).
- Repo-root KANBAN/KNOWLEDGE (upstream house rule).
- Purge-on-ACK (PDF draft) - breaks multi-favorite homes.
- L1-all-channels as default - mesh-hostile.
- New role enum - unnecessary.
- L3 peer sync - post-MVP.
