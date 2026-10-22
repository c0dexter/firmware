# L2-P4 design - nRF flash spill

_Status: P4.1 interface in progress (2026-10-22). Flash I/O not on RX hot path._

## Goals

- Keep **ESP RAM ring** as default (already ships).
- On **nRF**: SRAM holds index/cursors; payloads (or full lean records) spill to LittleFS/QSPI with wear-aware append.
- **Never** do flash I/O inside FloodingRouter / cancel path (L1). Ingest may enqueue to RAM then flush async if needed; MVP flash path can write on ingest if locked + bounded (prefer deferred flush in `runOnce`).

## Backend split

| Piece                           | Owns                       | Backend              |
| ------------------------------- | -------------------------- | -------------------- |
| Cursors / favorite ACL          | `ClientBaseSecretaryStore` | always SRAM          |
| Record ring (seq → lean record) | `ClientBaseSecretaryRing`  | RAM now; flash later |
| TTL / capacity / return_max     | Policy + Store             | unchanged            |

### Ring interface (locked)

```
init(capacity) / deinit / clear
capacity / size / nextSeq / oldestSeq
push(rec, nowSec, ttlSec) → seq   // expire + dropOldest inside
copyAtSeq(seq, out)
expire(nowSec, ttlSec)
```

Flash implementation must preserve **monotonic seq** and **oldest-first eviction** so cursors stay valid.

## nRF flash sketch (P4.2)

- Learn locking from `WarmNodeStore` (`FSCom._lockFS`, `spiLock` order).
- Append-only log of lean records + periodic compact; or index in SRAM + payload blobs by seq.
- Cap ring to MEM_CLASS / dump-time (existing Policy).
- Corruption → clear ring, keep cursors at nextSeq (safe empty history).

## Non-goals

- STM32WL L2 (excluded).
- Replacing global `StoreForwardModule`.
- Flash on L1 cancel path.
