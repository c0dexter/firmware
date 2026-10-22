# Status - CLIENT_BASE (snapshot)

_Last updated: 2026-10-22 - L2 dump enable fix + fake NodeDB stress; stretch = L1.5 log / 3rd-node dump TEXT_

## Branch / remote

|                   |                                         |
| ----------------- | --------------------------------------- |
| Branch            | `feature/client-base-role-improvements` |
| Fork tip (approx) | see `git log -1` on branch              |
| Push remote       | `fork` → `c0dexter/firmware`            |

## Done

| Layer        | What                                                                                                               | Commits (ref)                              |
| ------------ | ------------------------------------------------------------------------------------------------------------------ | ------------------------------------------ |
| **L1**       | Secondary never-cancel + late window; wire-hash fix on cancel path; Unity 8/8; sims + review; mesh wire-compatible | `855871142`, `5b81c9b5c`, docs `e4baaf0eb` |
| **L2 store** | `ClientBaseSecretaryStore` (lean 256 B, cursors, TTL); Unity 7/7                                                   | `f5e518782`                                |
| **L2-P0**    | Policy caps + module skeleton; Unity `test_client_base_l2_policy` 4/4                                              | `86799a79e`                                |
| **L2-P1**    | Secondary TEXT ingest → store; Unity `test_client_base_l2_ingest` 5/5                                              | `213665280`                                |
| **L2-P2**    | Poll session + S&F CLIENT_HISTORY dump; Unity `test_client_base_l2_poll` 6/6                                       | `70c07d09a`                                |
| **L2-P3**    | Caps constant + stats + schemas/UC/DoD polish                                                                      | `d697af008`                                |
| **M2**       | Primary elevate opt-in (runtime, default off); Unity L1 13/13; protobuf persist deferred                           | `cbeecd28f`                                |
| **L1.5**     | Free-hop / last-meter for favorites (`ClientBaseHop.h` + NextHopRouter); Unity `test_client_base_l15_hop` 8/8      | `fa6daed0a`                                |
| Docs/policy  | Analysis, sims, MVP/FVP, KB, journal; **D1=A** locked                                                              | `df318f8fb` + follow-ups                   |

## Next

1. **Desk P5 open:** L1.5 hop_start CDC proof; dump TEXT with third-party `from`; RF3 reverse. See `HW_TEST_RESULTS.md` / `MORNING_SUMMARY.md`.
2. Optional: M2 protobuf config key; L2-P4 nRF flash
3. Upstream PR (omit `.cursor/` if reviewers prefer)

## Explicitly not started

- Client app UX for poll
- L3 peer / phone BT sync
- Upstream PR to `meshtastic/firmware`

## Upstream baseline

See **[UPSTREAM_WATCH.md](UPSTREAM_WATCH.md)**.

- Build on **2.8.1 / `develop`**.
- **Before next L1 code:** open PR [#12180](https://github.com/meshtastic/firmware/pull/12180).
- **2.9 / 3.0:** #9942 roles, #10042 packets - watch.

## Resume checklist

1. Check #12180 merge state + rebase `develop`
2. **[L2_PLAN.md](L2_PLAN.md)** / **[UPSTREAM_WATCH.md](UPSTREAM_WATCH.md)**
3. This file + `JOURNAL.md` (top)
4. Skill: `.cursor/skills/client-base-secretary/SKILL.md`
