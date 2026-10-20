# Analysis & simulation — CLIENT_BASE household base

_Last updated: 2026-10-21_

Illustrative models only — **not** calibrated field RF measurements. Used to choose a mesh-safe default before implementation.

Scripts:

- `sim_abstract.py` — first Monte Carlo + secretary multi-reader
- `sim_variants.py` — multi-world policy comparison + system score
- `sim_platform_storage.py` — per-MEM_CLASS storage capacity + dump-time model

Re-run:

```bash
python3 .cursor/client-base-project/sim_abstract.py
python3 .cursor/client-base-project/sim_variants.py
python3 .cursor/client-base-project/sim_platform_storage.py
```

---

## 1. Problem analysis (code + field reports)

### User-reported failure

- Roof `CLIENT_BASE` + home node in **favorites**.
- Phone on roof **sees** channel messages (primary and secondary).
- Home node gets them **unreliably / “w kratkę”**.
- Operators escalate to `ROUTER` / `ROUTER_LATE`.

### Why phone sees but home does not

Two different paths:

1. **Local delivery** roof → phone (BLE/USB): needs only RX on roof.
2. **LoRa rebroadcast** roof → home: needs roof to **actually TX**.

### Root cause in firmware

| Mechanism | Behavior today |
|-----------|----------------|
| `roleAllowsCancelingDupe` | CLIENT_BASE cancels unless `isFromOrToFavoritedNode` |
| Broadcast (`to = broadcast`) | Favorite check uses **`from` only** |
| Mesh chat from others | `from` ≠ home → **not elevated** → CLIENT cancel-on-dupe |
| Hidden terminal | Distant node’s rebroadcast cancels roof TX; home hears neither source nor distant relay |
| Late window for favorites | Helps from/to favorite, **not** generic mesh→home broadcasts |

Favorites are overloaded as routing ACL and do **not** mean “deliver the channel to my household.”

### Product harm

Misleading role promise → frustration → **more full routers** on dense meshes (worse for the system than a scoped base).

---

## 2. Design options considered

| Option | Mesh impact | Household | Verdict |
|--------|-------------|-----------|---------|
| Do nothing | Low TX | Poor | Rejected |
| L1 never-cancel **all** channels | ≈ ROUTER_LATE public TX | Strong | Rejected as **default** (stealth router) |
| L1 never-cancel **secondary only** | Low public TX | Strong on private | **Default** |
| L1 secondary + **opt-in primary TEXT** | Moderate if enabled | Stronger on LongFast | **Opt-in** |
| L2 pull secretary (secondary) | Local unicast only | Offline catch-up | **MVP with L1** |
| L3 peer household sync | Extra LoRa later | After TTL | Deferred |
| New role enum | Docs/app churn | Same physics | Rejected — fix `CLIENT_BASE` |

---

## 3. Simulation assumptions (shared)

Urban-style knobs (example `urban_hidden`):

| Parameter | Value | Meaning |
|-----------|-------|---------|
| P(roof hears source) | ~0.92 | Strong attic RX |
| P(home hears source) | ~0.18 | Weak indoor |
| P(home hears distant relay) | ~0.12 | Hidden terminal |
| P(home hears roof) | ~0.88 | Roof→home OK |
| P(distant relay exists) | ~0.55–0.90 | Density |
| P(cancel race if CLIENT) | ~0.45–0.70 | Long client backoff |
| Private secondary fraction | ~0.12–0.20 | Of heard traffic |

Worlds in `sim_variants.py`: `urban_hidden`, `suburban`, `quiet_rural`, `dense_city_harsh`.

**System score** (higher = better for mesh + household):

`2 × home_delivery% − 1.4 × public_TX% − 0.3 × all_roof_TX%`

---

## 4. Results — live bridge (`sim_abstract.py`)

N = 5000 packets, single urban-like world:

| Policy | Home delivery | Roof TX | Cancel |
|--------|---------------|---------|--------|
| CLIENT_BASE today | ~63% | ~55% | ~37% |
| L1 late never-cancel (all) | ~86% | ~92% | 0% |
| ROUTER_LATE / ROUTER | ~86% | ~92% | 0% |

Airtime proxy (TX slots / 100 heard):

| Policy | Slots |
|--------|------:|
| CLIENT_BASE today | ~54 |
| L1 all channels | ~92 |
| **L1 secondary-only + CLIENT public** | **~60** |
| ROUTER / ROUTER_LATE | ~92 |

**Private-only slice** (urban_hidden): today ~59% → L1 secondary ~86% home delivery.

---

## 5. Results — multi-world ranking (`sim_variants.py`)

Aggregate system score across four worlds:

| Rank | Policy | Aggregate score |
|-----:|--------|----------------:|
| 1 | **l1_secondary_only** | ~209 |
| 2 | cb_today | ~187 |
| 3 | l1_secondary_plus_primary_text | ~184 |
| … | router_late / l1_all / router | ~152–154 |

Interpretation:

- **Best for the system:** scoped L1 on secondary (fixes private without city-scale public TX).
- **L1-all ≈ ROUTER** on public airtime — do not ship as default.
- **Primary TEXT opt-in** is the compromise if operators need LongFast→home and accept cost.

---

## 6. Results — secretary / multi-reader

| Scenario | Outcome |
|----------|---------|
| Anna pull then Bart +2h (cursors) | Bart still receives shared history |
| Same with purge-on-first-ACK | Bart misses Anna’s batch (~24 msgs in model) |
| Chris after TTL 24h | Only recent window; pre-TTL needs L3 or longer TTL |
| L2 airtime | Scales with **favorites × unread**, not mesh size |
| Home offline 6h | L1 recovers **0**; L1+L2 recovers buffered secondary (e.g. 24) at local unicast cost |

---

## 7. Locked policy (accepted)

1. **Default L1:** late rebroadcast, **never cancel** for traffic on **secondary channels configured on the base**.
2. **Default L2:** buffer those secondaries (TEXT-focused); **per-favorite cursor**; TTL/ring; no purge-on-first-ACK.
3. **Opt-in config:** elevate **primary TEXT** only (default **off**). Honest docs if off: primary remains best-effort CLIENT-like.
4. **Forbidden as default:** L1 on all channels / stealth ROUTER_LATE.
5. **Keep role name** `CLIENT_BASE`; favorites = **served** household ACL.
6. **L3** deferred; **performance:** no flash I/O on RX cancel path; hard caps on records, favorites, poll rate.

---

## 8. Network safety statement

Safe **if** scoped as above: household reliability on private secondaries + local pull, without defaulting every attic base into a public mesh amplifier.

Unsafe **if** L1-all is the default: same public TX profile that already harms dense meshes when users install ROUTER out of frustration.

---

## 9. Platform storage & dump simulation (`sim_platform_storage.py`)

Previously we only described ESP vs nRF qualitatively. This section models **usable secretary budgets** by Meshtastic `MEM_CLASS` (`src/memory/MemClass.h`), not raw chip marketing RAM.

### Firmware facts used

| Fact | Value |
|------|------:|
| `DATA_PAYLOAD_LEN` | 233 |
| Full S&F-like record (approx) | ~296 B |
| **Lean L2 record (recommended)** | **256 B** |
| SRAM index entry (nRF) | ~20 B |

Budgets are **conservative reserved** quotas (after NodeDB/radio), not entire heap/flash.

### Capacity @ 4 TEXT msg/h on secondary (LEAN records)

| Platform | Class | Backend | N records | Hours @ 4/h | Fits 24h TTL? |
|----------|-------|---------|----------:|------------:|:-------------:|
| STM32WL | TINY | unsupported | 0 | 0 | no — **L2 exclude** |
| nRF52840 (RAK4631, …) | SMALL | flash + SRAM index | ~409 | ~102 | **yes** |
| RP2040/RP2350 | SMALL | flash + index | ~512 | ~128 | **yes** |
| ESP32 / C3 (no PSRAM) | SMALL | RAM ring ~32 KiB | ~128 | ~32 | **yes** |
| ESP32-S3 no PSRAM | MEDIUM | RAM ~64 KiB | ~256 | ~64 | **yes** |
| ESP32-S3 + PSRAM | LARGE | PSRAM 512 KiB quota | ~2048 | ~512 | **yes** (must **cap**) |

### Dump behavior (why caps matter)

Assume stop-and-wait unicast ~2.5 s/msg + ACK (abstract LongFast-ish):

| Scenario | Local channel busy |
|----------|-------------------:|
| One favorite pulls 24h window (96 msg) | **~4 min** |
| 3 favorites serialized, same window | **~12 min** |
| 8 favorites serialized | **~32 min** |

PSRAM can store far more than users should dump in one sitting → **hard `history_return_max`** and per-class `max_favorites` (≈6–12).

### Platform conclusions (design locks)

1. Ship **LEAN_RECORD**, not full `PacketHistoryStruct`, on SMALL/MEDIUM.
2. **nRF52840 is viable** for L2 via QSPI/LittleFS + SRAM index (classic S&F is ESP-oriented/`EXCLUDE` on nRF — we do not rely on it).
3. **ESP without PSRAM** can hold TTL 24h at modest TEXT rates in a small RAM ring; optional flash spill.
4. **ESP+PSRAM** is easy on capacity — risk is **dump airtime**, not bytes; default caps mandatory.
5. **STM32WL:** no L2 secretary (TINY).
6. Flash wear: low msg rate is OK with ring + bounded partition; still require wear-aware writes.
7. Multi-favorite dumps must be **queued/serialized** with poll rate limits (same busy model as S&F).
