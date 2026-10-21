# CLIENT_BASE - household base (live + secretary)

Working design hub for the `feature/client-base-role-improvements` branch on fork `c0dexter/firmware`.

This folder is the **project knowledge plane** (not upstream Meshtastic docs). It must stay current while we develop. Upstream documentation will later be distilled into the PR description and [meshtastic/meshtastic](https://github.com/meshtastic/meshtastic) if/when upstreamed.

**Current snapshot:** **[STATUS.md](STATUS.md)** - L1 closed; L2 store done; L2 module/poll **paused**.  
**Next work:** **[L2_PLAN.md](L2_PLAN.md)** (phased plan ready to execute).  
**Upstream alignment:** **[ALIGNMENT_WITH_UPSTREAM.md](ALIGNMENT_WITH_UPSTREAM.md)** (nasze vs #12180 / #10704 - ryzyko review).

## Goal

Make `CLIENT_BASE` match user expectation: a roof/attic **base** that serves favorited household nodes - without forcing operators onto `ROUTER` / `ROUTER_LATE`, and **without** turning every base into a city-scale public amplifier.

## Locked policy (from analysis + simulation)

| Piece                    | Decision                                                                                   |
| ------------------------ | ------------------------------------------------------------------------------------------ |
| Role name                | Keep `CLIENT_BASE` (fix + upgrade, not a new role)                                         |
| **L1 default**           | Late, never-cancel on **secondary channels present on the base**                           |
| **L1 opt-in**            | Elevate **primary TEXT** only - default **off**                                            |
| **L1 forbidden default** | Never-cancel on **all** channels (≈ ROUTER_LATE airtime)                                   |
| **L2**                   | Pull-on-demand secretary; shared ring; **per-favorite cursor**; TTL; no purge-on-first-ACK |
| **L3**                   | Household peer catch-up - deferred                                                         |
| Safety north star        | Prefer scoped household TX over public mesh flood                                          |

Full write-up: **[ANALYSIS_AND_SIMULATION.md](ANALYSIS_AND_SIMULATION.md)**  
Scripts: `sim_abstract.py`, `sim_variants.py`, `sim_platform_storage.py` (MEM_CLASS capacity + dump time)

## Layers

| Layer | Name          | MVP? | Intent                                                                              |
| ----- | ------------- | ---- | ----------------------------------------------------------------------------------- |
| L1    | Live bridge   | Yes  | Secondary (default): home hears private traffic the roof got; no CLIENT cancel race |
| L2    | Secretary     | Yes  | Buffer those secondaries; on-demand unicast to favorites                            |
| L3    | Peer catch-up | FVP  | After roof TTL - app BT/Wi‑Fi sync first; LoRa peer optional                        |

## MVP vs FVP

See **[MVP_AND_FVP.md](MVP_AND_FVP.md)**. Journal: **[JOURNAL.md](JOURNAL.md)**.

|           | MVP                                                                | FVP                                                          |
| --------- | ------------------------------------------------------------------ | ------------------------------------------------------------ |
| Meaning   | Smallest safe fix of the role promise                              | Full household-secretary product                             |
| Must have | L1 secondary + L2 cursors/TTL + caps + tests                       | MVP + app UX + hardening + L3 catch-up + field/upstream docs |
| Status    | **L1 closed**. L2 store done. **L2 module paused** (see STATUS.md) | After MVP: F1→F7 in `MVP_AND_FVP.md`                         |

## Non-goals (MVP)

- Replacing global `StoreForwardModule` (PSRAM mesh S&F).
- New device role enum.
- Purge buffer on first client ACK.
- Default behavior equivalent to ROUTER / ROUTER_LATE on the public primary.
- Phone↔phone BT sync (that is **FVP / F4**, not firmware MVP).

## Artifacts

| File                         | Purpose                                          |
| ---------------------------- | ------------------------------------------------ |
| `STATUS.md`                  | One-page current snapshot (start here on resume) |
| `L2_PLAN.md`                 | Phased L2 secretary implementation plan          |
| `UPSTREAM_WATCH.md`          | FW baseline vs develop; Meshtastic 3.0 watch     |
| `ANALYSIS_AND_SIMULATION.md` | Problem analysis, sim results, locked policy     |
| `MVP_AND_FVP.md`             | MVP scope, done criteria, FVP roadmap            |
| `JOURNAL.md`                 | Append-only work log                             |
| `KNOWLEDGE_BASE.md`          | Living decisions / findings                      |
| `KANBAN.md`                  | Text board                                       |
| `TODO.md`                    | Checklist                                        |
| `use-cases.md`               | Scenarios                                        |
| `schemas.md`                 | Flow diagrams                                    |
| `sim_*.py`                   | Abstract simulations                             |

## Remotes

- Push: `fork` → `https://github.com/c0dexter/firmware`
- Upstream fetch: `origin` → `meshtastic/firmware`
