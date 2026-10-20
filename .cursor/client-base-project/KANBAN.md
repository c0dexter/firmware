# Kanban — CLIENT_BASE

Sync manually with GitHub Project when available.

## Backlog

- [x] Lock L1 policy (advisor): default secondary-only late never-cancel; opt-in primary TEXT; never default L1-all
- [x] Multi-variant sim (`sim_variants.py`) across 4 worlds + system score
- [x] Abstract Monte Carlo sim (`sim_abstract.py`) + conclusions in KB
- [ ] L2 protobuf / poll message sketch (MVP)
- [ ] L2 storage API sketch ESP vs nRF (MVP)
- [ ] GitHub Project board + columns
- [x] MVP_AND_FVP.md written

## Todo (MVP)

- [ ] MVP: Unity tests current vs desired L1 (secondary never-cancel)
- [ ] MVP: implement L1 + tests
- [ ] MVP: L2 ring + per-favorite cursor + poll/unicast + tests
- [ ] MVP: caps (`history_return_max`, max favorites) + docs PR-ready

## Backlog (FVP — after MVP)

- [ ] F1 primary UX in apps
- [ ] F2 client protocol polish (Android/iOS/web)
- [ ] F3 storage hardening / wear
- [ ] F4 phone BT/Wi‑Fi history sync
- [ ] F5 optional LoRa peer catch-up
- [ ] F6 metrics + upstream docs
- [ ] F7 field validation

## In progress

- [x] Agent rules + skills + project knowledge plane bootstrap

## Verification

- [ ] Native tests green for L1
- [ ] Native tests for cursor/TTL (L2)
- [ ] Performance notes recorded (hot path, flash wear)

## Done

- [x] First commit: docs/rules/skills/sims on fork branch (`da47cfd2a`)
- [x] Audit CLIENT_BASE behavior vs docs
- [x] Problem analysis (misleading favorites / ROUTER escape hatch)
- [x] Architecture direction: L1+L2, keep role name, multi-cursor buffer
- [x] Fork `c0dexter/firmware` + branch `feature/client-base-role-improvements`
- [x] Abstract + multi-variant simulations; policy lock
- [x] Docs updated: ANALYSIS_AND_SIMULATION.md + README/KB/use-cases/schemas/rules/skills sync
- [x] Platform storage/dump sim (`sim_platform_storage.py`) + §9 in ANALYSIS
