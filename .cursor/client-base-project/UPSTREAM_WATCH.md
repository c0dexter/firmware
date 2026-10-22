# Upstream watch - baseline, 2.8/2.9 CLIENT_BASE, 3.0

_Last updated: 2026-10-22_

## What we build on today

| Item                  | Value                                                                         |
| --------------------- | ----------------------------------------------------------------------------- |
| Branch                | `feature/client-base-role-improvements` @ fork                                |
| Upstream default      | **`develop`** (not `master`)                                                  |
| Lineage               | **`v2.8.1`** included; tip near current `develop`                             |
| Latest public release | **2.8.1 Alpha** ([releases](https://github.com/meshtastic/firmware/releases)) |

Hygiene: rebase/merge `origin/develop` before long coding sessions.  
**2026-10-22:** merged `origin/develop` into feature branch (clean; Channels auto-merged). Our L1 secondary + L1.5 + L2 dump Unity still green.

---

## Priority: overlaps with **our** L1/L2 (act on these)

### P0 - Competing L1 live-relay work (2.8.x)

Same root pain as UC1 (broadcast cancel-on-dupe → indoor misses). **Different fix shape.**

| Ref                                                                                             | Status                                                   | Approach                                                                                                                                                                                         | vs our L1                                                        |
| ----------------------------------------------------------------------------------------------- | -------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ---------------------------------------------------------------- |
| [#12156](https://github.com/meshtastic/firmware/issues/12156) CLIENT_BASE Improvements          | Open                                                     | Proposals: 0-hop late fill-in ± power backoff; or TEXT-only elevate                                                                                                                              | Validates problem; airtime concern same as ours                  |
| **[#12180](https://github.com/meshtastic/firmware/pull/12180)** fill-in for favorited neighbors | **Still OPEN** (2026-10-22), `MERGEABLE`, targets `develop` | On **all broadcasts**: if CLIENT_BASE has **direct** favorite (`hops_away==0`), **don’t cancel** on foreign dupe → late window; cancel only if favorite itself relayed (`resolveUniqueLastByte`) | **Conflict / merge risk** with our secondary-scoped never-cancel |
| [#10704](https://github.com/meshtastic/firmware/issues/10704) roof S&F / pull missing msgs      | Closed stale                                             | Pull history from roof (0-hop answer)                                                                                                                                                            | **Aligns with our L2** direction                                 |

**Our L1 (locked):** never-cancel + late only on **configured SECONDARY**; primary stays CLIENT-like (opt-in TEXT later = M2).

**Their #12180:** never-cancel (late fill-in) on **any broadcast** when a direct favorite exists - including **public primary**. Closer to scoped ROUTER_LATE while someone is “home”; field data shows large delivery win and still less TX than full ROUTER_LATE when favorite proves RX.

#### Locked stance (2026-10-22)

1. **#12180 not merged** → keep shipping **secondary-scoped** L1 (sim winner). No L1 code changes until it lands or we consciously adopt a complement.
2. **When #12180 merges:** rebase; prefer **Complement** (keep secondary guarantee without needing hops_away==0; let #12180 cover primary when favorite is direct). Do **not** replace our default with all-channel fill-in.
3. Optional: comment on #12156/#12180 linking `ANALYSIS_AND_SIMULATION.md` secondary-only score (operator call).

### P1 - L2 validation

| Ref                                    | Note                                                                           |
| -------------------------------------- | ------------------------------------------------------------------------------ |
| #10704                                 | Community already asked for roof pull / S&F-like 0-hop dump - supports L2 plan |
| Comments on #12156 (TEXT-only elevate) | Close to our M2 / secondary TEXT focus                                         |

### P2 - Hops / “free hop” (**L1.5 shipped on fork** - field-validate with P5)

| Ref                                                         | Note                                                                                                                                                                                                                 |
| ----------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [#8366](https://github.com/meshtastic/firmware/issues/8366) | CLIENT_BASE shouldn’t eat hop for favorites. **Shipped on fork (L1.5)** 2026-10-21: `ClientBaseHop.h` FreeHopBumpStart + DeliverAtZero in `NextHopRouter`; Unity `test_client_base_l15_hop`. Field-validate with P5. |

---

## Label **2.9** (thin public list)

Only two issues currently labeled `2.9`:

| Issue                                                                                                   | Relevance                                                                                                                                                                                         |
| ------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [#9942](https://github.com/meshtastic/firmware/issues/9942) reduce roles (also **3.0**)                 | **Keep Client_base** in proposed minimal set; UX rename (“Home”); possible later deprecation if replay exists                                                                                     |
| [#8347](https://github.com/meshtastic/firmware/issues/8347) mandatory-rebroadcast only via remote admin | Targets **ROUTER / ROUTER_LATE** (infra abuse). CLIENT_BASE mentioned in comments as useful roof role - **unlikely to gate CLIENT_BASE** the same way; watch so household base stays app-settable |

**2.9 is not a shipped train** in the same sense as 2.8.1 releases - treat as planning labels + #9942/#8347 watch.

---

## Label **3.0** (next major cluster)

Not released; no date. Themes that touch us:

| Theme                    | Refs           | Impact                                                                         |
| ------------------------ | -------------- | ------------------------------------------------------------------------------ |
| Packet / header / replay | #10042, #11766 | Re-test L1 cancel/late after header changes; L2 “replay” dumps may need a flag |
| Role reduction / rename  | #9942          | Port `ClientBaseRelay` / role checks; behavior > brand                         |
| nRF filesystem           | #4447          | Before L2-P4 flash                                                             |
| Telemetry nested         | #9489…         | Low for TEXT secretary                                                         |
| Radio region breaking    | EU433 / JP     | Low for role logic                                                             |

---

## Impact matrix (summary)

| Our work                      | Upstream risk                                             | Action                                                      |
| ----------------------------- | --------------------------------------------------------- | ----------------------------------------------------------- |
| **L1 secondary never-cancel** | **High** - #12180 may land on develop / 2.8.3             | Rebase + reconcile before more L1; document complementarity |
| L1 hash→index fix             | Low (correctness)                                         | Keep regardless of #12180                                   |
| **L2 secretary**              | Medium-positive (#10704); protocol TBD                    | Continue plan; watch replay flag for 3.0                    |
| M2 primary TEXT opt-in        | Medium - overlaps #12156 TEXT-only / #12180 all-broadcast | May become redundant or become “narrower than #12180”       |
| Role name forever             | Medium 2.9/3.0                                            | Policy helpers stay role-agnostic where possible            |

---

## Recommendation

1. **Do not stop** the project - problem is confirmed upstream with field numbers.
2. Before next coding: **`git fetch` + inspect #12180 merge state**; update this file.
3. Keep locked default **secondary-scoped** unless we deliberately change policy after comparing airtime to #12180.
4. L2 remains differentiating (pull / multi-cursor) vs live fill-in PRs.
5. 2.9/3.0 = watch + rebase discipline, not a rewrite today.

---

## Sources

- `origin/develop`, tag `v2.8.1.8e6a88d`
- https://github.com/meshtastic/firmware/labels/2.9
- https://github.com/meshtastic/firmware/labels/3.0
- https://github.com/meshtastic/firmware/issues/12156
- https://github.com/meshtastic/firmware/pull/12180
- https://github.com/meshtastic/firmware/issues/10704
- https://github.com/meshtastic/firmware/issues/9942
- https://github.com/meshtastic/firmware/issues/8347
- https://github.com/meshtastic/firmware/issues/8366
- https://github.com/meshtastic/firmware/issues/10042
- https://github.com/meshtastic/firmware/issues/11766
