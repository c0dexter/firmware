# Status - CLIENT_BASE (snapshot)

_Last updated: 2026-10-21 - session paused before L2 module work_

## Branch / remote

|                   |                                         |
| ----------------- | --------------------------------------- |
| Branch            | `feature/client-base-role-improvements` |
| Fork tip (approx) | `e4baaf0eb` + later status commit       |
| Push remote       | `fork` → `c0dexter/firmware`            |

## Done

| Layer        | What                                                                                                               | Commits (ref)                              |
| ------------ | ------------------------------------------------------------------------------------------------------------------ | ------------------------------------------ |
| **L1**       | Secondary never-cancel + late window; wire-hash fix on cancel path; Unity 8/8; sims + review; mesh wire-compatible | `855871142`, `5b81c9b5c`, docs `e4baaf0eb` |
| **L2 store** | `ClientBaseSecretaryStore` (lean 256 B, cursors, TTL); Unity 7/7 - **not wired to RX/module yet**                  | `f5e518782`                                |
| Docs/policy  | Analysis, sims, MVP/FVP, KB, journal, rules/skills                                                                 | `df318f8fb` + follow-ups                   |

## Paused / next when resuming

1. **L2 module** - ingest secondary TEXT → store; favorites ACL; poll → unicast; Unity
2. **Protobuf decision** - reuse StoreAndForward vs new messages (biggest schedule risk)
3. Caps (`history_return_max`, max favorites)
4. **M2** - opt-in primary TEXT (default off)
5. nRF flash backend (after RAM path works)
6. Hardware smoke UC1; PR polish

**FVP (F1-F7)** - after MVP only.

## Explicitly not started

- L2 poll on-air / client apps
- L3 peer / phone BT sync
- Upstream PR to `meshtastic/firmware`

## Resume checklist

1. Read `JOURNAL.md` (top entries) + this file
2. Read `KNOWLEDGE_BASE.md` § L1 / L2
3. Skill: `.cursor/skills/client-base-secretary/SKILL.md`
4. Continue from KANBAN **In progress** / TODO L2 module line
