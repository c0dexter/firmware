# CLIENT_BASE hardware desk test results

_Last run: 2026-10-22 ~00:25 UTC+2 · Branch tip: `fa6daed0a` · Harness: `scripts/desk_smoke.py`_

## Scope (desk - what we can / cannot prove)

| Layer                  | In scope on desk (2 nodes)                                              | Out of scope / needs field                                                      |
| ---------------------- | ----------------------------------------------------------------------- | ------------------------------------------------------------------------------- |
| **Config / identity**  | Roles, LongFast, EU_868, secondary `household`, favorites               | App UX                                                                          |
| **RF link**            | Bidirectional TEXT SNR/hops on shared channels                          | Hidden-terminal / cancel race vs city mesh                                      |
| **L1 live**            | Secondary present on base; favorite path exists                         | Cancel-on-dupe under multi-router contention                                    |
| **L1.5 free-hop**      | Precondition: favorite TX with `hopStart==hopLimit` at roof             | Serial `FreeHopBumpStart` log (CDC silent in protobuf mode); 3rd-node hop audit |
| **L2 secretary**       | Poll ACL (unfavorite → silent); ingest+dump needs working SF reply path | Multi-favorite UC5; offline window UC6 hours                                    |
| **Memory / stability** | `rebootCount` under TX burst                                            | Long soak, PSRAM edge, brownout                                                 |

**Test types:** functional RF · policy/ACL · hop metadata · poll protocol · negative (non-fav) · stability/memory · config regression.

---

## Hardware under test

| Role               | Board       | Port (this run) | FW                          | Config                                                     |
| ------------------ | ----------- | --------------- | --------------------------- | ---------------------------------------------------------- |
| Roof `CLIENT_BASE` | Heltec V4   | `/dev/ttyACM3`  | `2.8.2.fa6daed`             | CB Roof / LongFast / EU_868 / ch1 `household`              |
| Home `CLIENT`      | Heltec T114 | `/dev/ttyACM2`  | `2.8.2.fa6daed` (clean UF2) | Home Fav `!ebba7ec6` / LongFast / EU_868 / ch1 `household` |

T114 recovered after wipe + clean UF2 (earlier DFU left CDC dead).

---

## Results matrix (`desk_smoke.py`)

| ID       | Test                                                     | Status    | Evidence / notes                                                                           |
| -------- | -------------------------------------------------------- | --------- | ------------------------------------------------------------------------------------------ |
| **S0**   | FW identity + roles (CLIENT_BASE=12, CLIENT=0, LongFast) | **PASS**  | health snapshot                                                                            |
| **S1**   | Secondary `household` on both                            | **PASS**  | ch index 1 role SECONDARY                                                                  |
| **S2**   | Mutual favorite                                          | **PASS**  | `!ebba7ec6`<→`!da4db613`                                                                   |
| **RF1**  | home→roof **primary** TEXT                               | **PASS**  | SNR≈6.75 dB, `hopStart=3=hopLimit`, RSSI≈−34                                               |
| **RF2**  | home→roof **secondary** TEXT (timed wait)                | **FAIL*** | Race in harness; later stress/L15 secondary RX **did** arrive on roof with `channel` field |
| **RF3**  | roof→home secondary TEXT                                 | **FAIL**  | No RX on home in window; reverse RF flaky or listen race - retest                          |
| **L15a** | Favorite direct RF to CLIENT_BASE                        | **PASS**  | Roof RX `hopStart==hopLimit==3` (FreeHop precondition)                                     |
| **L15b** | Observe FreeHopBumpStart / hop bump on echo              | **SKIP**  | No self-echo; raw CDC capture 0 B in protobuf mode                                         |
| **L2a**  | Favorite `CLIENT_HISTORY` poll → dump/header             | **PASS*** | After enable-fix flash: `ROUTER_HISTORY` on air; self-seeds → Empty header (skip-own)      |
| **L2b**  | Non-favorite poll (3rd node)                             | **SKIP**  | Covered partially by L2c                                                                   |
| **L2c**  | Poll while **unfavorited** → no dump                     | **PASS**  | `replies=0` (ACL silent reject)                                                            |
| **MEM1** | Roof `rebootCount` under 25× secondary TX                | **PASS**  | `3→3`, home `0→0`, tx_ok=25                                                                |
| **MEM2** | Fake NodeDB fill (`add_contact` ×60)                     | **PASS**  | nodedb 45→82; reboot stable under fill; L2 header still works                              |
| **M2**   | Primary elevate default off                              | **PASS**  | Config/contract; Unity owns matrix                                                         |

\*RF2 counted FAIL in automation; manual correlation upgrades secondary RF to **PASS (degraded confidence)**.

**Harness tally:** PASS 8 · FAIL 2 · WARN 1 · SKIP 2 (raw). With RF2 reinterpreted: effective secondary RF OK.

Artifacts: `/tmp/cb_desk_smoke/summary.json`, `events.jsonl`, `run.log`.

---

## Edge cases exercised

| Case                                                       | Outcome                                                          |
| ---------------------------------------------------------- | ---------------------------------------------------------------- |
| Unfavorite then poll                                       | Silent reject (**PASS**)                                         |
| Self-TX does not fill L2 for same node’s “from us” on roof | By design (`isFromUs` skip); desk seeds must be **home→roof** RX |
| Primary vs secondary path                                  | Primary RX solid; secondary RX seen under load                   |
| TX burst / airtime                                         | No reboot; SNR stayed ~6 dB                                      |
| Clean T114 vs dirty DFU                                    | Clean UF2 restores USB CDC (important process note)              |

---

## Analysis

### What works on the desk

1. **Two-node LongFast mesh** after T114 clean flash: primary TEXT home→roof is reliable (~6 dB SNR, 0 hops).
2. **CLIENT_BASE identity + secondary channel ACL surface** match the locked policy.
3. **L1.5 precondition** is visible in packet metadata: favorite originator arrives with `hopStart == hopLimit` (direct RF). That is exactly the FreeHopBumpStart gate in `ClientBaseHop.h`.
4. **Stability:** no reboot under short secondary flood (MEM1).
5. **L2 ACL:** unfavorite poll does not produce a dump (matches silent-reject code).

### Gaps / defects to chase

1. ~~L2 poll silent~~ → **fixed overnight:** dump thread needed `enabled=true` after ctor `disable()`; skip `from==requester` for Empty/dump honesty. Header E2E PASS; third-party dump TEXT still needs 3rd node.
2. **L1.5 not log-proven** on device (no CDC text). Need `noproto`/log viewer or 3rd sniffer for `hop_start++`.
3. **RF3 roof→home** failed in harness - may be channel timing, RX duty, or harness listen; not yet a firmware verdict.

### Memory / crash susceptibility (this session)

- No OOM reboot on V4 during 25-msg burst (MEM1) or 60 fake contacts (MEM2, nodedb→82).
- `rebootCount` rises only on intentional flashes (5→7 overnight).
- Home still favorite after fake fill + unfavorite cleanup.

---

## Conclusions

| Claim                                                      | Desk verdict                                                   |
| ---------------------------------------------------------- | -------------------------------------------------------------- |
| Branch boots on Heltec V4 + T114                           | **Yes**                                                        |
| CLIENT_BASE + household secondary + favorites configurable | **Yes**                                                        |
| Favorite CLIENT → roof RF path live                        | **Yes** (primary proven; secondary observed under load)        |
| L1.5 free-hop **behavior** proven                          | **Partial** - gate inputs yes; hop_start bump not yet observed |
| L2 secretary ingest+poll E2E                               | **Header yes** - dump TEXT needs non-self `from` (3rd node)    |
| FW crash under light / fake-NodeDB stress                  | **No**                                                         |

**Next desk actions:** (1) commit L2 fixes; (2) L1.5 noproto/sniffer; (3) 3rd-node dump TEXT; (4) RF3 retry.

**Field later:** UC1 cancel race, L1.5 hop budget across multi-hop, UC6 offline buffer.

---

## Earlier session notes (archived)

- First T114 DFU left USB CDC dead → wipe + UF2 fixed.
- V4 LongFast set via BLE when serial contested by stress job.
- Classic `store_forward.enabled` is **False** on V4 (secretary not blocked by D1 gate).
