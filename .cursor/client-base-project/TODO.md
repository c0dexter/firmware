# TODO - CLIENT_BASE project

## Immediate (tooling)

- [x] Project rules under `.cursor/rules/`
- [x] Skills under `.cursor/skills/`
- [x] Knowledge plane under `.cursor/client-base-project/`
- [x] ANALYSIS_AND_SIMULATION.md + doc sync after policy lock
- [x] Platform memory/dump simulation documented
- [ ] Re-auth `gh` (`gh auth login`) and create GitHub Project; link URL here
- [x] Track `.cursor/` on fork (docs/rules/skills); omit from upstream PR if needed
- [x] Journal (`JOURNAL.md`) started

## MVP (see MVP_AND_FVP.md)

- [x] Abstract + multi-variant + platform sims; policy lock
- [x] Define MVP vs FVP in docs
- [x] Unity tests: L1 secondary never-cancel + policy helpers (`test_client_base_l1`)
- [x] Implement L1 + tests (FloodingRouter + Channels + ClientBaseRelay.h); wire-hash fix; milestone closed on fork
- [x] L2 RAM store + cursors + TTL Unity (`ClientBaseSecretaryStore`)
- [ ] L2 module: ingest secondary TEXT + poll/unicast + tests - **NEXT when resuming** (paused 2026-10-21)
- [ ] L2 protobuf / poll sketch (decide S&F reuse vs new msgs)
- [ ] L2 nRF flash backend (RAM ok for first ESP path)
- [ ] M2 opt-in primary TEXT config (default off)
- [ ] Caps + honest primary opt-in docs; PR-ready

## FVP (later)

- [ ] F1-F7 per `MVP_AND_FVP.md` (apps, hardening, phone BT sync, optional LoRa peer, field)

## Ongoing

- [x] After every work chunk: update KANBAN + KNOWLEDGE_BASE + JOURNAL + this file
- [x] Before every commit: run `project-status-commit` ritual
