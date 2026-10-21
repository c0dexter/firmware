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
- [ ] L2-P1: ingest secondary TEXT + Unity - **NEXT**
- [ ] L2-P2: poll/unicast + tests (D1=A)
- [ ] L2-P3: caps/docs polish
- [ ] L2 nRF flash backend (RAM ok for first ESP path)
- [ ] M2 opt-in primary TEXT config (default off)
- [ ] Caps + honest primary opt-in docs; PR-ready

## Adjacent (not blocking L2; do after module or with M2)

- [ ] **L1.5 free-hop for favorites** - dom→dach nie powinien zjadać skoku z budżetu mesh (#8366)
  - Today: `Router::shouldDecrementHopLimit` - first hop always decrement; preserve only when previous relay is unambiguous favorite ROUTER / ROUTER_LATE / CLIENT_BASE
  - Desired: when local is `CLIENT_BASE` and packet is from / via a **favorite household CLIENT**, consider not burning that hop (or equivalent scoped rule)
  - Fits our ACL story (favorites = served household); not attic=ROUTER
  - Design gate: loop / infinite hop-preserve safety (why first-hop always decrements today)
  - Tests: Unity around `shouldDecrementHopLimit` + CLIENT_BASE + favorite CLIENT cases
  - Track: `UPSTREAM_WATCH.md` P2; notes in KB

## FVP (later)

- [ ] F1-F7 per `MVP_AND_FVP.md` (apps, hardening, phone BT sync, optional LoRa peer, field)

## Ongoing

- [x] After every work chunk: update KANBAN + KNOWLEDGE_BASE + JOURNAL + this file
- [x] Before every commit: run `project-status-commit` ritual
