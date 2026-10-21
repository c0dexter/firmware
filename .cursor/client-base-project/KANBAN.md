# Kanban - CLIENT_BASE

Sync manually with GitHub Project when available.

## Backlog

- [x] Lock L1 policy (advisor): default secondary-only late never-cancel; opt-in primary TEXT; never default L1-all
- [x] Multi-variant sim (`sim_variants.py`) across 4 worlds + system score
- [x] Abstract Monte Carlo sim (`sim_abstract.py`) + conclusions in KB
- [x] L2 execution plan written (`L2_PLAN.md`) - D1 protobuf still open at start of coding
- [x] L2 RAM storage API (`ClientBaseSecretaryStore`); nRF flash spill still open (L2-P4)
- [ ] GitHub Project board + columns
- [x] MVP_AND_FVP.md written
- [ ] M2: opt-in primary TEXT config (default off)
- [ ] **L1.5 / free-hop:** `CLIENT_BASE` nie pali `hop_limit` dla ulubionych CLIENT (dom→dach→mesh); dziś `shouldDecrementHopLimit` chroni tylko favorite ROUTER/CLIENT_BASE + first hop zawsze −1 - [#8366](https://github.com/meshtastic/firmware/issues/8366); po L2 module / obok M2; ostrożnie z pętlami

## Todo (MVP)

- [x] MVP: Unity tests current vs desired L1 (secondary never-cancel)
- [x] MVP: implement L1 + tests
- [x] MVP: L2 ring + per-favorite cursor store + Unity (`ClientBaseSecretaryStore`)
- [ ] MVP: L2 module ingest + poll/unicast + tests
- [ ] MVP: caps (`history_return_max`, max favorites) + docs PR-ready

## Backlog (FVP - after MVP)

- [ ] F1 primary UX in apps
- [ ] F2 client protocol polish (Android/iOS/web)
- [ ] F3 storage hardening / wear
- [ ] F4 phone BT/Wi‑Fi history sync
- [ ] F5 optional LoRa peer catch-up
- [ ] F6 metrics + upstream docs
- [ ] F7 field validation

## In progress

- [ ] ~~MVP L2 module + poll/unicast~~ → **PAUSED** - resume via `L2_PLAN.md` / `STATUS.md`

## Verification

- [x] Native tests green for L1 (`test_client_base_l1`, coverage env, 8/8 incl. wire-hash fix)
- [x] Post-change sims re-run + L1 cancel-path code review
- [x] Native tests for cursor/TTL (L2 store) - `test_client_base_l2_store` 7/7
- [x] Upstream watch: 2.8 #12180 / 2.9 roles / 3.0 packet - `UPSTREAM_WATCH.md`
- [ ] On resume: rebase `develop` + reconcile L1 vs PR #12180
- [ ] Performance notes recorded (hot path, flash wear)

## Done

- [x] First commit: docs/rules/skills/sims on fork branch
- [x] Audit CLIENT_BASE behavior vs docs
- [x] Problem analysis (misleading favorites / ROUTER escape hatch)
- [x] Architecture direction: L1+L2, keep role name, multi-cursor buffer
- [x] Fork `c0dexter/firmware` + branch `feature/client-base-role-improvements`
- [x] Abstract + multi-variant simulations; policy lock
- [x] Docs updated: ANALYSIS_AND_SIMULATION.md + README/KB/use-cases/schemas/rules/skills sync
- [x] Platform storage/dump sim (`sim_platform_storage.py`) + §9 in ANALYSIS
- [x] Agent rules + skills + project knowledge plane bootstrap
- [x] **L1 live bridge** - `ClientBaseRelay.h` + `isConfiguredSecondary` / `isPacketOnConfiguredSecondary` + FloodingRouter + Unity 8/8 (incl. wire-hash fix) - **milestone closed**
- [x] **L2 store core** - `ClientBaseSecretaryStore` + Unity multi-reader/TTL
