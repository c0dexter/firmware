# Priority queue - CLIENT_BASE

_Locked 2026-10-22. Change only with an explicit decision + KB note._

| P | Item | Why this order | Status |
|---|------|----------------|--------|
| **P0** | Upstream hygiene: watch #12180; merge `origin/develop` regularly | Avoid L1 merge debt / surprise conflicts | **Done this session** (#12180 still open; develop merged) |
| **P1** | **L2-P4** nRF flash backend (interface → SRAM index + LittleFS/QSPI) | L2 firmware MVP stretch; ESP RAM already ships | **Next** |
| **P2** | **M2** protobuf persist for primary elevate opt-in | Runtime flag exists; apps need a key | After P1 first slice |
| **P3** | UC6 offline hours field proof | Desk stretch; needs long soak or scripted clock | When operator schedules |
| **P4** | Client UX for `ROUTER_TEXT_*` / upstream PR polish | FVP / product boundary | Later |

**Do not** start new L1 behavior until #12180 disposition is recorded again after it merges.
