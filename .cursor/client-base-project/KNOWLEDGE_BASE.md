# Knowledge base - CLIENT_BASE project

_Last updated: 2026-10-21 (L1 + L2 store core)_

Canonical long-form analysis: **[ANALYSIS_AND_SIMULATION.md](ANALYSIS_AND_SIMULATION.md)**.  
Work log: **[JOURNAL.md](JOURNAL.md)**.

## Decisions (accepted)

1. **Keep role name `CLIENT_BASE`** - solid fix + secretary upgrade, not a new role.
2. **Root user pain:** favorites elevate only `from`/`to`; channel broadcasts from the mesh are not elevated → roof phone sees messages, home often does not (cancel-on-dupe + hidden terminal) → operators install ROUTER/ROUTER_LATE.
3. **L1 + L2 together:** live fix for in-range home; secretary for offline / missed window. L2 alone is insufficient.
4. **L1 default (LOCKED):** late never-cancel only on **secondary channels configured on the base**. Wins multi-world system score; lowest public TX among effective fixes.
5. **L1 opt-in (LOCKED):** config to also elevate **primary TEXT** - default **off**. Honest UX: without it, primary stays best-effort CLIENT-like. **Not wired yet** (M2).
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
- `ClientBaseRelay.h` - `clientBaseAllowsCancelingDupe(fav, secondary)` / `clientBaseUsesLateRebroadcastWindow(...)` (pure, testable).
- `FloodingRouter`: CLIENT_BASE never-cancels and clamps late window when favorite **or** configured secondary; primary non-favorite still CLIENT cancel.
- Tests: `test/test_client_base_l1/` - 8/8 (includes wire-hash regression).

### Post-change review (2026-10-21)

- Sims re-run: policy lock still holds (`l1_secondary_only` aggregate winner).
- Caught and fixed hash-vs-index bug on L1 cancel path (see JOURNAL).

### L2 store core (current)

- `ClientBaseSecretaryStore` - calloc ring + fixed cursor slots; lean 256 B records; `push` / `expire` / `ensureFavorite` / `pullNext` (cursor-only advance).
- Tests: `test/test_client_base_l2_store/` - 7/7 (multi-reader no purge, TTL, capacity, favorite cap).
- Still open: ProtobufModule wiring, poll RR, unicast TX, flash spill for nRF, `history_return_max` session cap.

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
- Protobuf: extend StoreAndForward vs new private control messages (client impact).
- Exact config key name for primary TEXT opt-in.
- GitHub Project board ID after `gh auth login`.
- Upstream PRs: consider omitting `.cursor/` if reviewers prefer; keep on fork.

## Rejected / deferred

- Mandatory visible `<thinking>` every reply (original mega-prompt).
- Repo-root KANBAN/KNOWLEDGE (upstream house rule).
- Purge-on-ACK (PDF draft) - breaks multi-favorite homes.
- L1-all-channels as default - mesh-hostile.
- New role enum - unnecessary.
- L3 peer sync - post-MVP.
