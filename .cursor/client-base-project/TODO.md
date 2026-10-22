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
- [x] L2-P5: hardware smoke (header + 3-node dump `ROUTER_TEXT_*` + MEM2); UC6 offline hours still field
- [x] Commit overnight L2 enable + skip-own fixes
- [x] Dump wire: SF `ROUTER_TEXT_*` + hop `has_bitfield` (legacy DM / pre-hop drop)
- [x] Unity: pin dump wire (`test_client_base_l2_dump` 3/3) + shrink-to-fit Data.payload
- [x] L2-P4.1 ring interface + RAM (`ClientBaseSecretaryRing.h` / L2_P4_DESIGN.md)
- [ ] L2-P4.2 nRF flash spill - **QUEUE P1 remaining**
- [x] M2 opt-in primary TEXT elevate (runtime; default off)
- [ ] M2 protobuf config key + app surface (persist) - QUEUE P2
- [ ] Caps + honest primary opt-in docs; PR-ready - with upstream PR
- [x] Merge `origin/develop` (2026-10-22); #12180 still open → L1 stance locked (QUEUE / UPSTREAM_WATCH)
- [ ] UC6 offline hours field proof - QUEUE P3

## Adjacent (not blocking L2)

- [x] **L1.5 free-hop for favorites** - `ClientBaseHop.h` + NextHopRouter; Unity `test_client_base_l15_hop` (#8366)
- [x] L1.5 desk proof: FreeHopBumpStart via `security.debug_log_api_enabled` (`hop_start=4 hop_limit=3`)

## FVP (later)

- [ ] F1-F7 per `MVP_AND_FVP.md` (apps, hardening, phone BT sync, optional LoRa peer, field)

## Ongoing

- [x] After every work chunk: update KANBAN + KNOWLEDGE_BASE + JOURNAL + this file
- [x] Before every commit: run `project-status-commit` ritual
