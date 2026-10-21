# Status - CLIENT_BASE (snapshot)

_Last updated: 2026-10-21 - paused; L2 execution plan ready (`L2_PLAN.md`)_

## Branch / remote

|                   |                                          |
| ----------------- | ---------------------------------------- |
| Branch            | `feature/client-base-role-improvements`  |
| Fork tip (approx) | see `git log -1` on branch (plan commit) |
| Push remote       | `fork` → `c0dexter/firmware`             |

## Done

| Layer        | What                                                                                                               | Commits (ref)                              |
| ------------ | ------------------------------------------------------------------------------------------------------------------ | ------------------------------------------ |
| **L1**       | Secondary never-cancel + late window; wire-hash fix on cancel path; Unity 8/8; sims + review; mesh wire-compatible | `855871142`, `5b81c9b5c`, docs `e4baaf0eb` |
| **L2 store** | `ClientBaseSecretaryStore` (lean 256 B, cursors, TTL); Unity 7/7 - **not wired to RX/module yet**                  | `f5e518782`                                |
| Docs/policy  | Analysis, sims, MVP/FVP, KB, journal, rules/skills                                                                 | `df318f8fb` + follow-ups                   |

## Paused / next when resuming

Follow **[L2_PLAN.md](L2_PLAN.md)** in order:

1. Lock **D1** (S&F reuse vs new protobuf) + confirm **D2** caps
2. L2-P0 module skeleton → L2-P1 ingest → L2-P2 poll/unicast → L2-P3 caps/docs
3. Optional L2-P4 nRF flash, L2-P5 hardware smoke
4. Later: M2 primary TEXT opt-in; FVP F1-F7

## Explicitly not started

- L2 module / on-air poll / client apps
- L3 peer / phone BT sync
- Upstream PR to `meshtastic/firmware`

## Upstream baseline

See **[UPSTREAM_WATCH.md](UPSTREAM_WATCH.md)**.

- Build on **2.8.1 / `develop`**.
- **Before next L1 code:** open PR [#12180](https://github.com/meshtastic/firmware/pull/12180) (broadcast fill-in ~2.8.3) - reconcile with our secondary-scoped L1.
- **2.9 / 3.0:** #9942 roles, #10042 packets - watch; L2 secretary still distinct (#10704).

## Resume checklist

1. Check #12180 merge state + rebase `develop`
2. **[L2_PLAN.md](L2_PLAN.md)** / **[UPSTREAM_WATCH.md](UPSTREAM_WATCH.md)**
3. This file + `JOURNAL.md` (top)
4. Skill: `.cursor/skills/client-base-secretary/SKILL.md`
