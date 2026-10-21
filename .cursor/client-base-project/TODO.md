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
- [x] L2 plan documented (`L2_PLAN.md`)
- [x] L2 D1: lock poll protocol → **A** (S&F RR reuse; no classic S&F co-enable)
- [x] L2-P0: policy caps + module skeleton + Unity `test_client_base_l2_policy`
- [x] L2-P1: ingest secondary TEXT + Unity `test_client_base_l2_ingest`
- [x] L2-P2: poll/unicast + Unity `test_client_base_l2_poll` (D1=A)
- [x] L2-P3: caps/docs/stats/schema polish
- [ ] L2-P5: hardware smoke UC6 (stretch)
- [ ] L2 nRF flash backend (RAM ok for first ESP path)
- [x] M2 opt-in primary TEXT elevate (runtime; default off)
- [ ] M2 protobuf config key + app surface (persist)
- [ ] Caps + honest primary opt-in docs; PR-ready

## Adjacent (not blocking L2)

- [x] **L1.5 free-hop for favorites** - `ClientBaseHop.h` + NextHopRouter; Unity `test_client_base_l15_hop` (#8366)
- [ ] L1.5 field check with P5 hardware smoke (dom→dach hop_start bump)

## FVP (later)

- [ ] F1-F7 per `MVP_AND_FVP.md` (apps, hardening, phone BT sync, optional LoRa peer, field)

## Ongoing

- [x] After every work chunk: update KANBAN + KNOWLEDGE_BASE + JOURNAL + this file
- [x] Before every commit: run `project-status-commit` ritual
