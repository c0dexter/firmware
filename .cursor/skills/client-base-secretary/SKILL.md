---
name: client-base-secretary
description: >-
  Designs and implements CLIENT_BASE live-relay fixes and pull-on-demand
  household secretary storage. Use when working on CLIENT_BASE, favorites
  coverage, cancel-on-dupe, late rebroadcast, private secondary buffering,
  poll/unicast delivery, ESP32/nRF storage backends, or household base UX.
---

# CLIENT_BASE secretary

## Before changing code

1. Read `.cursor/client-base-project/STATUS.md`, `L2_PLAN.md` (if touching L2), `KNOWLEDGE_BASE.md`, `KANBAN.md`, `use-cases.md`.
2. Re-read current semantics in `FloodingRouter`, `Router::shouldDecrementHopLimit`, `NodeDB::isFromOrToFavoritedNode`, existing `StoreForwardModule` (reference only - different product).
3. State which layer you touch: **L1 live**, **L2 secretary**, or docs/tests only.
4. For L2: lock plan **D1** (protocol) before poll TX; follow phases P0→P3.

## Layer contracts

### L1 - live bridge (LOCKED)

- Problem: broadcast elevated only if `from` is favorite → cancel-on-dupe + hidden terminal; phone on roof sees RX, home misses.
- **Default:** late never-cancel on **secondary channels configured on the base** only.
- **Opt-in:** primary TEXT elevate (default off). **Never** default all-channel elevate.
- Hot path: no flash I/O in dupe/cancel decision.
- Evidence: `ANALYSIS_AND_SIMULATION.md` / `sim_variants.py` (secondary-only wins system score).

### L2 - secretary (LOCKED)

- Buffer TEXT on **secondary channels present on the base**.
- Shared ring; **per-NodeNum cursor**; TTL/capacity eviction - **never** delete because one favorite ACKed.
- Poll from favorite → local unicast; busy/queue for multi-client.
- ESP: RAM/PSRAM with **hard caps** (dump time). nRF: SRAM index + LittleFS/QSPI (viable). STM32WL: no L2.
- Use LEAN ~256 B records; see `sim_platform_storage.py` / ANALYSIS §9.
- Cap `history_return_max` and max favorites - full buffer dump ≈ minutes of local airtime.

### L3 - peer catch-up

- Out of MVP. Document only in knowledge base if discussed.

## Performance checklist

- [ ] RX path alloc-free
- [ ] Hard caps: records, served favorites, poll interval
- [ ] nRF: no large payload arrays in SRAM
- [ ] Extra airtime justified and scoped (channel / role)

## Tests

Add/extend Unity tests for cancel/relay decisions and cursor/TTL behavior under `test/`. Update kanban when tests land.
