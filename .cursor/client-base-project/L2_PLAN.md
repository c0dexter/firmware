# L2 work plan - CLIENT_BASE secretary

_Ready to execute when operator resumes. Policy lock: `ANALYSIS_AND_SIMULATION.md`. Store already landed: `ClientBaseSecretaryStore`._

**Goal:** favorited household nodes can **pull** buffered secondary TEXT from the roof without purge-on-ACK and without becoming a city ROUTER.

**Out of this plan:** L3, phone BT sync, full app polish (FVP), replacing global `StoreForwardModule`.

---

## 0. Preconditions (already true)

| Item                                                              | Status                                          |
| ----------------------------------------------------------------- | ----------------------------------------------- |
| L1 secondary live bridge                                          | Done (`855871142` + hash fix `5b81c9b5c`)       |
| Lean ring + per-favorite cursors + TTL Unity                      | Done (`f5e518782`, `test_client_base_l2_store`) |
| Policy: secondary TEXT only; favorites ACL; no purge-on-first-ACK | Locked                                          |
| STM32WL                                                           | No L2 (skip at `#ifdef` / MEM_CLASS)            |

**Resume entry:** this file + `STATUS.md` + skill `client-base-secretary`.

---

## 1. Decision gate (do first - blocks schedule)

### D1 - Poll protocol (pick one before coding module RX/TX)

| Option                                                                                    | Pros                                                   | Cons                                                                                              | Recommendation                                                                                                                                 |
| ----------------------------------------------------------------------------------------- | ------------------------------------------------------ | ------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| **A. Reuse `STORE_FORWARD_APP` / `StoreAndForward` RR** (`CLIENT_HISTORY`, text payloads) | No protobufs PR; apps that already speak S&F may adapt | Semantics differ (global S&F vs household); risk of confusing two products; S&F often PSRAM-gated | **MVP default if we want speed** - gate: only when `role==CLIENT_BASE` and requester is favorite; never enable classic S&F server on same node |
| **B. New private portnum / messages** (protobufs repo)                                    | Clean product boundary                                 | Wait for protobufs + client sync; longer calendar                                                 | Prefer for **upstream-ready** MVP / F2                                                                                                         |
| **C. Temporary TEXT control + binary payload**                                            | Zero proto churn                                       | Ugly; hard to ship upstream                                                                       | Reject for anything beyond a spike                                                                                                             |

**Decision to record in KB when starting:** A or B.  
**Spike allowed:** implement ingest-only (phase 2) while D1 is open; **do not** ship poll TX until D1 is locked.

### D2 - Default caps (propose; tune later)

| Cap                                   | Proposed MVP default                                                         | Rationale                           |
| ------------------------------------- | ---------------------------------------------------------------------------- | ----------------------------------- |
| Record size                           | 256 B lean (done)                                                            | sim_platform_storage                |
| Ring capacity                         | ESP no-PSRAM ~64-128; ESP+PSRAM capped ≤256 usable for dump; nRF flash later | dump time                           |
| TTL                                   | 24 h                                                                         | locked sim                          |
| `history_return_max` per poll session | **32** (hard)                                                                | ~1-2 min local airtime @ ~2.5 s/msg |
| Max served favorites (cursor slots)   | **8**                                                                        | airtime × favorites                 |
| Inter-packet delay while dumping      | reuse S&F-like spacing (~packetTimeMax)                                      | airtime fairness                    |
| Busy                                  | one dump client at a time; others get BUSY / retry                           | multi-reader                        |

---

## 2. Phased implementation

### Phase L2-P0 - Module skeleton (½-1 session)

**Deliverable:** `ClientBaseSecretaryModule` (name TBD) registered only when `config.device.role == CLIENT_BASE` and not STM32WL / exclude flag.

| Task | Detail                                                                                           |
| ---- | ------------------------------------------------------------------------------------------------ |
| P0.1 | New module under `src/modules/` owning a `ClientBaseSecretaryStore` instance                     |
| P0.2 | `init(capacity, maxFavorites, ttlSec)` from D2 + MEM_CLASS helpers                               |
| P0.3 | Sync cursor slots from NodeDB favorites on config/NodeDB changes (ensureFavorite / drop removed) |
| P0.4 | `#define` / CMake / Modules.cpp wiring; no-op on non-CLIENT_BASE                                 |
| P0.5 | Unity: module constructs + init under native (smoke)                                             |

**Exit:** builds on `native`/`coverage` + one ESP env; store empty but alive.

### Phase L2-P1 - Ingest (1 session)

**Deliverable:** secondary TEXT that the base decrypts is appended to the ring.

| Task | Detail                                                                                                                |
| ---- | --------------------------------------------------------------------------------------------------------------------- |
| P1.1 | `wantPacket` / `handleReceived`: `TEXT_MESSAGE_APP`, channel via `isPacketOnConfiguredSecondary` / index after decode |
| P1.2 | Map `MeshPacket` → `ClientBaseLeanRecord` (from, id, channel, time, payload truncated to 236)                         |
| P1.3 | Skip: primary (unless later M2), non-TEXT, encrypted opaque we cannot decode, our own TX if undesired                 |
| P1.4 | Hot path: **no flash I/O** on ingest (RAM ring only in MVP-first)                                                     |
| P1.5 | Unity: ingest mock packets → `availableCount` for a registered favorite                                               |

**Exit:** UC6 buffer fills in unit tests without poll yet.

### Phase L2-P2 - Poll + unicast (1-2 sessions; needs D1)

**Deliverable:** favorite requests history → base unicasts up to `history_return_max`, advancing **that** cursor only.

| Task | Detail                                                                                                         |
| ---- | -------------------------------------------------------------------------------------------------------------- |
| P2.1 | ACL: reject poll if `!nodeDB->isFavorite(from)` (and optionally not in cursor table)                           |
| P2.2 | Implement D1 protocol handlers (HISTORY request / BUSY / text frames / done)                                   |
| P2.3 | Session state: `busyTo`, packetsSentThisSession, respect `history_return_max`                                  |
| P2.4 | Delivery: unicast on **same secondary channel** as stored record (or agreed control channel - document choice) |
| P2.5 | Multi-client: second poller → BUSY; do not purge ring                                                          |
| P2.6 | Unity: Anna pull then Bart pull (UC5); offline window pull (UC6); favorite-only ACL                            |

**Exit:** UC5/UC6 pass at module level (not only store).

### Phase L2-P3 - Caps, observability, docs (½ session)

| Task | Detail                                                                                    |
| ---- | ----------------------------------------------------------------------------------------- |
| P3.1 | Enforce D2 caps in one place (constants / small config struct)                            |
| P3.2 | LOG_INFO/DEBUG counters: enqueued, pulled, busy, expired (no spam on RX hot path)         |
| P3.3 | Update `schemas.md` sequence for poll; JOURNAL + KB; honest UC2 still primary best-effort |
| P3.4 | `trunk fmt`; `pio test -e coverage -f test_client_base_l2*`                               |

### Phase L2-P4 - nRF flash backend (optional for first ESP ship)

| Task | Detail                                                        |
| ---- | ------------------------------------------------------------- |
| P4.1 | Interface: ring ops behind backend (RAM vs flash+index)       |
| P4.2 | nRF: index in SRAM, payloads LittleFS/QSPI; wear-aware append |
| P4.3 | Unity + dry-run capacity checks                               |

**Note:** MVP can ship ESP RAM-only; nRF L2 follows P4.

### Phase L2-P5 - Hardware smoke (operator + MCP)

| Scenario                                                                | Pass                                                        |
| ----------------------------------------------------------------------- | ----------------------------------------------------------- |
| Roof CLIENT_BASE + secondary; home favorite; home offline 10+ min; poll | Home receives buffered TEXT; second favorite still can pull |
| Primary without M2                                                      | No L2 store of primary; L1 still CLIENT-like cancel         |
| Non-favorite poll                                                       | Rejected / ignored                                          |

---

## 3. Files likely touched

| Path                                            | Role                                  |
| ----------------------------------------------- | ------------------------------------- |
| `src/modules/ClientBaseSecretaryModule.{h,cpp}` | **new** - ingest + poll               |
| `src/modules/Modules.cpp`                       | register                              |
| `src/mesh/ClientBaseSecretaryStore.*`           | extend if session/API gaps            |
| `src/mesh/Channels.*`                           | reuse `isPacketOnConfiguredSecondary` |
| `test/test_client_base_l2_*`                    | ingest + poll suites                  |
| `.cursor/client-base-project/*`                 | JOURNAL/KB/STATUS after each phase    |
| `protobufs` / generated                         | **only if D1=B**                      |

**Do not:** edit `src/mesh/generated/` by hand; change global S&F defaults for all roles.

---

## 4. Test matrix

| Suite                              | Covers                                      |
| ---------------------------------- | ------------------------------------------- |
| `test_client_base_l2_store`        | Ring/cursors/TTL (exists)                   |
| `test_client_base_l2_ingest` (new) | TEXT secondary → store; primary ignored     |
| `test_client_base_l2_poll` (new)   | ACL, multi-cursor, history_return_max, busy |
| `test_client_base_l1`              | No regression                               |

---

## 5. Risks & mitigations

| Risk                          | Mitigation                                                            |
| ----------------------------- | --------------------------------------------------------------------- |
| Proto/app lag (D1=B)          | Start P0-P1 on A or ingest-only; swap transport later                 |
| Dump airtime                  | Hard `history_return_max`; serialize clients                          |
| Confusion with S&F (D1=A)     | Role gate + favorite ACL + docs name “household secretary”            |
| Hash/index mistakes on ingest | Ingest runs **post-decode** (index); cancel path already fixed for L1 |
| Flash wear (nRF)              | Defer P4; RAM first                                                   |

---

## 6. Definition of done (L2 firmware MVP)

- [ ] CLIENT_BASE only; secondary TEXT buffered in lean store
- [ ] Favorites can poll; cursors independent; no purge-on-ACK
- [ ] Caps enforced; busy behavior defined
- [ ] Unity green for ingest + poll suites
- [ ] Docs: STATUS/JOURNAL/KB + schema poll diagram
- [ ] D1 decision recorded in KB
- [ ] (Stretch) one hardware smoke UC6

M2 (primary TEXT opt-in) and upstream PR are **adjacent**, not required to call L2 firmware slice done.

---

## 7. Suggested session order

1. Lock **D1** + confirm **D2** numbers in KB (15-30 min).
2. **L2-P0** skeleton.
3. **L2-P1** ingest + tests → commit.
4. **L2-P2** poll + tests → commit.
5. **L2-P3** caps/docs → commit.
6. Optional **L2-P4** / **L2-P5** when hardware available.

Estimate (agent): ~1-2 focused sessions for P0-P3 if D1=A; longer if D1=B waits on protobufs.
