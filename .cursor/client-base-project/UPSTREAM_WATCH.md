# Upstream watch - version baseline & 3.0

_Last updated: 2026-10-21 (analysis only; no code change)_

## What we build on today

| Item                         | Value                                                                                                                                             |
| ---------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| Branch                       | `feature/client-base-role-improvements` @ fork                                                                                                    |
| Tip (then)                   | `716496d6d`                                                                                                                                       |
| `git describe`               | `v2.8.1.8e6a88d-117-g…` - **includes release tag 2.8.1**                                                                                          |
| Upstream default             | **`develop`** (not `master`)                                                                                                                      |
| Distance to `origin/develop` | **~2 commits behind** when checked (essentially current)                                                                                          |
| Latest public release stream | **2.8.1 Alpha** ([releases](https://github.com/meshtastic/firmware/releases)) - 2.8.0 revoked (signing); 2.8.1 signatures incompatible with 2.8.0 |

**Verdict:** we are on the **2.8.x / current `develop` line**, not an old 2.6/2.7 fork. Rebase/merge from `origin/develop` before the next big L2 push is still good hygiene (~2 commits drift).

`master` can look “stale” vs our tip - ignore for day-to-day; track **`develop`**.

---

## Meshtastic 3.0 - what is publicly known

**3.0 is not a shipped product.** It is a GitHub label / holding area for **next-major, often breaking** work. No official release date found. Conversation often points to Discord + labeled issues.

Useful entry points:

- Label: https://github.com/meshtastic/firmware/labels/3.0
- Packet structure placeholder: https://github.com/meshtastic/firmware/issues/10042
- Unofficial radar (not Meshtastic-owned): https://canadaverse.org/meshtastic/releases/

### Themes clustered under 3.0 (open / draft)

| Theme                               | Example                                | Relevance to CLIENT_BASE L1/L2                                                                                              |
| ----------------------------------- | -------------------------------------- | --------------------------------------------------------------------------------------------------------------------------- |
| **Packet / header structure**       | #10042, “replay” flag #11766           | **High** if cancel/dupe/rebroadcast semantics or header fields change                                                       |
| **Role list reduction**             | #9942 (also labeled **2.9** + **3.0**) | **High** - proposes keeping CLIENT_BASE but may rename/repurpose; mentions possible later deprecation if “replay” covers it |
| Nested telemetry protos             | #9489, #11520                          | Low for our L1/L2 TEXT path                                                                                                 |
| nRF FS reliability                  | #4447                                  | Medium for **L2-P4 flash** backend                                                                                          |
| EU433 / JP freq breaking radio defs | #2696, #11559                          | Low for role logic; regional                                                                                                |
| Module rename cleanup               | #11122                                 | Low                                                                                                                         |
| Protobufs channel MQTT flags move   | protobufs #395                         | Low for relay policy                                                                                                        |

### Role issue that matters to us (#9942)

Proposal (discussion, **not merged**): shrink roles; **keep Client_base** in the reduced set; optional future deprecation of `client_base` and/or `router` **if** replay/other mechanisms allow. Alternate UX naming (“Home” roof/attic = former CLIENT_BASE) appears in comments.

→ Our L1/L2 work still targets the **household base behavior**; if 2.9/3.0 renames the enum, we port policy helpers - do **not** stop L2, but avoid assuming the string `CLIENT_BASE` is eternal in UX docs.

---

## Impact assessment (for this project)

| Layer                          | 3.0 risk now                                      | Action                                                                 |
| ------------------------------ | ------------------------------------------------- | ---------------------------------------------------------------------- |
| L1 (cancel / late / secondary) | Low-medium: depends on packet/dupe path stability | Keep helpers pure (`ClientBaseRelay.h`); re-test after develop rebases |
| L2 store (RAM ring)            | Low                                               | Stable                                                                 |
| L2 poll protocol               | Medium if new portnums / S&F changes in major     | Prefer D1 design that can swap transport                               |
| L2 nRF flash                   | Watch #4447                                       | Read before P4                                                         |
| Role name / enum               | Medium on 2.9/3.0 horizon                         | Track #9942; policy = behavior, not brand                              |

**Recommendation:** continue L2 on **2.8 / develop**. Treat 3.0 as a **watch list**, not a blocker. Refresh this file when #10042 / #9942 move or a 3.0 milestone gets a date.

---

## Sources checked

- Local: `git describe`, `origin/develop` vs `HEAD`, tag `v2.8.1.8e6a88d`
- https://github.com/meshtastic/firmware/releases
- https://github.com/meshtastic/firmware/labels/3.0
- https://github.com/meshtastic/firmware/issues/9942
- https://github.com/meshtastic/firmware/issues/10042
- https://canadaverse.org/meshtastic/releases/ (unofficial)
