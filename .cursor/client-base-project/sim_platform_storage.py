#!/usr/bin/env python3
"""
Platform storage / dump simulation for CLIENT_BASE L2 secretary.

Uses Meshtastic firmware facts:
  - meshtastic_Constants_DATA_PAYLOAD_LEN = 233
  - MEM_CLASS tiers from src/memory/MemClass.h
  - PacketHistoryStruct-like full record ≈ sizeof used by StoreForward
  - Lean secretary record (recommended) stores less metadata

NOT a device emulator — budget model for design caps.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import List, Tuple

PAYLOAD = 233  # meshtastic_Constants_DATA_PAYLOAD_LEN

# Approximate sizeof(PacketHistoryStruct) with typical alignment (~280-300B).
# Measured layout estimate:
FULL_RECORD = (
    4  # time
    + 4  # bool+pad / has_rx_time cluster
    + 4  # to
    + 4  # from
    + 4  # id
    + 4  # channel + pad
    + 4  # reply_id
    + 4  # emoji + pad
    + PAYLOAD  # payload[233]
    + 4  # payload_size (+pad)
    + 4  # rx_rssi
    + 4  # has_rx_rssi + pad
    + 4  # rx_snr float
    + 4  # hops / flags cluster
)  # ≈ 285; use 296 with safety
FULL_RECORD = 296

# Lean L2 record: time, from, id, channel, payload_size, payload, flags
LEAN_RECORD = 4 + 4 + 4 + 1 + 1 + 2 + PAYLOAD + 4  # ≈ 253; pad to 256
LEAN_RECORD = 256

# nRF index entry in SRAM (msg_id, channel, time, file_offset, size)
INDEX_ENTRY = 4 + 1 + 4 + 4 + 2 + 1  # ≈ 16; pad 20
INDEX_ENTRY = 20


@dataclass(frozen=True)
class Platform:
    name: str
    mem_class: str  # TINY/SMALL/MEDIUM/LARGE
    examples: str
    # Usable budget reserved for secretary (not entire chip RAM)
    sram_budget_b: int  # index and/or in-RAM ring
    flash_budget_b: int  # 0 if RAM-only ring
    backend: str  # "ram" | "flash+index" | "psram"
    # Practical defaults we would ship
    max_favorites: int
    notes: str


# Conservative usable budgets AFTER NodeDB/radio/UI (design caps, not chip totals).
PLATFORMS: List[Platform] = [
    Platform(
        "STM32WL",
        "TINY",
        "Seeed Wio-E5, etc.",
        sram_budget_b=0,
        flash_budget_b=0,
        backend="unsupported",
        max_favorites=0,
        notes="MEM_CLASS_TINY — L2 secretary not viable; L1-only or exclude",
    ),
    Platform(
        "nRF52840",
        "SMALL",
        "RAK4631, T-Echo, many Nordic boards (QSPI flash common)",
        sram_budget_b=8 * 1024,  # index only
        flash_budget_b=256 * 1024,  # dedicated LittleFS quota (shared FS — stay modest)
        backend="flash+index",
        max_favorites=8,
        notes="No PSRAM; classic S&F excluded on nRF — flash ring is the path",
    ),
    Platform(
        "RP2040/RP2350",
        "SMALL",
        "RPi Pico based variants",
        sram_budget_b=24 * 1024,
        flash_budget_b=128 * 1024,
        backend="flash+index",
        max_favorites=6,
        notes="Watchdog/write limits; keep caps low",
    ),
    Platform(
        "ESP32 classic / C3",
        "SMALL",
        "Heltec V2/V3 (non-S3), C3 boards",
        sram_budget_b=32 * 1024,  # lean RAM ring if no PSRAM
        flash_budget_b=64 * 1024,  # optional LittleFS spill
        backend="ram",
        max_favorites=6,
        notes="Tight heap; prefer LEAN_RECORD; optional flash spill",
    ),
    Platform(
        "ESP32-S3 no PSRAM",
        "MEDIUM",
        "Some S3 boards without PSRAM",
        sram_budget_b=64 * 1024,
        flash_budget_b=128 * 1024,
        backend="ram",
        max_favorites=8,
        notes="RAM ring LEAN; flash optional overflow",
    ),
    Platform(
        "ESP32-S3 + PSRAM",
        "LARGE",
        "Heltec V3/V4 many, T-Deck, etc.",
        sram_budget_b=16 * 1024,  # cursors/state
        flash_budget_b=0,
        backend="psram",
        max_favorites=12,
        notes="PSRAM ring like StoreForward; budget 512KiB PSRAM for secretary",
    ),
]

# Extra PSRAM budget modeled separately for LARGE
PSRAM_SECRETARY_BUDGET = 512 * 1024

MSG_PER_HOUR = 4  # private secondary TEXT
AVG_PAYLOAD = 80  # typical text < full 233
TTL_H = 24
# Airtime abstract: full packet + want_ack round (~ LongFast-ish)
SEC_PER_MSG_UNICAST = 2.5
SEC_PER_POLL_CTRL = 1.0


def capacity_records(p: Platform, record_size: int) -> Tuple[int, str]:
    if p.backend == "unsupported":
        return 0, "n/a"
    if p.backend == "flash+index":
        n_flash = p.flash_budget_b // record_size
        n_index = p.sram_budget_b // INDEX_ENTRY
        n = min(n_flash, n_index)
        return n, f"min(flash={n_flash}, index={n_index})"
    if p.backend == "ram":
        n = p.sram_budget_b // record_size
        if p.flash_budget_b:
            n = max(n, p.flash_budget_b // record_size)  # if spill used, flash dominates
            # actually for ram primary: use sram; flash is overflow — report sram primary
            n = p.sram_budget_b // record_size
        return n, "sram_ring"
    if p.backend == "psram":
        n = PSRAM_SECRETARY_BUDGET // record_size
        return n, f"psram {PSRAM_SECRETARY_BUDGET // 1024}KiB"
    return 0, "?"


def hours_cover(n_records: int, rate: float = MSG_PER_HOUR) -> float:
    if rate <= 0:
        return 0.0
    return n_records / rate


def dump_seconds(n_msgs: int, favorites_pulling: int = 1) -> float:
    # serialized busy queue: each favorite dumps sequentially in worst case
    return favorites_pulling * (SEC_PER_POLL_CTRL + n_msgs * SEC_PER_MSG_UNICAST)


def flash_writes_per_day(rate: float = MSG_PER_HOUR) -> int:
    return int(rate * 24)


def main() -> None:
    print("=== RECORD SIZES ===")
    print(f"DATA_PAYLOAD_LEN={PAYLOAD}")
    print(f"FULL_RECORD(S&F-like)≈{FULL_RECORD} B")
    print(f"LEAN_RECORD(L2)≈{LEAN_RECORD} B")
    print(f"INDEX_ENTRY≈{INDEX_ENTRY} B")
    print()

    print("=== CAPACITY BY PLATFORM (LEAN records) ===")
    print(
        f"{'platform':22s} {'class':7s} {'backend':13s} {'Nrec':>6} {'~hours@4/h':>11} "
        f"{'24h_fit':>7} {'dump_1fav':>10} {'dump_3fav':>10}"
    )
    rows = []
    for p in PLATFORMS:
        n, how = capacity_records(p, LEAN_RECORD)
        hrs = hours_cover(n)
        fit24 = "YES" if n >= MSG_PER_HOUR * TTL_H else "NO"
        # dump of full buffer or 24h window
        window = min(n, MSG_PER_HOUR * TTL_H)
        d1 = dump_seconds(window, 1)
        d3 = dump_seconds(window, 3)
        rows.append((p, n, hrs, fit24, d1, d3, how))
        print(
            f"{p.name:22s} {p.mem_class:7s} {p.backend:13s} {n:6d} {hrs:11.1f} "
            f"{fit24:>7} {d1/60:9.1f}m {d3/60:9.1f}m"
        )

    print()
    print("=== FULL_RECORD vs LEAN (selected) ===")
    for name in ("nRF52840", "ESP32 classic / C3", "ESP32-S3 + PSRAM"):
        p = next(x for x in PLATFORMS if x.name == name)
        nl, _ = capacity_records(p, LEAN_RECORD)
        nf, _ = capacity_records(p, FULL_RECORD)
        print(f"  {name}: LEAN N={nl} ({hours_cover(nl):.0f}h)  FULL N={nf} ({hours_cover(nf):.0f}h)")

    print()
    print("=== nRF FLASH WEAR (abstract) ===")
    wpd = flash_writes_per_day()
    # Assume 100k cycle NOR page; ring spreads writes — order-of-magnitude years
    cycles = 100_000
    pages_touched_factor = 4  # wear leveling imperfect
    years = cycles / (wpd * pages_touched_factor / 1) / 365
    print(f"  writes/day≈{wpd} (1 record/msg)")
    print(f"  naive longevity proxy ≈{years:.0f}+ years at low rate (still: bound partition + wear-aware ring)")

    print()
    print("=== MULTI-FAVORITE DUMP CONTENTION ===")
    nrf = next(x for x in PLATFORMS if x.name == "nRF52840")
    n, _ = capacity_records(nrf, LEAN_RECORD)
    window = min(n, MSG_PER_HOUR * TTL_H)
    for fav in (1, 2, 3, 5, 8):
        sec = dump_seconds(window, fav)
        print(f"  nRF full 24h window ({window} msg) × {fav} favorites serialized: {sec/60:.1f} min channel busy local")

    print()
    print("=== CONCLUSIONS ===")
    print("1) Use LEAN_RECORD (256B), not full S&F struct, on SMALL/MEDIUM.")
    print("2) nRF52840: flash+index can hold 24h@4msg/h if flash quota ≥~25KiB payloads+index — 256KiB quota is comfortable.")
    print("3) ESP32 no-PSRAM: 32KiB RAM ≈ 128 lean msgs ≈ 32h@4/h — OK for TTL 24h if TEXT-only.")
    print("4) ESP32+PSRAM: hundreds–thousands of msgs — do not default huge; cap for dump time.")
    print("5) Dump time dominates UX: 96 msg × 2.5s ≈ 4 min; serialize favorites; cap history_return_max.")
    print("6) STM32WL: exclude L2 secretary.")
    print("7) Cap max_favorites per class (6–12) to bound worst-case local airtime.")


if __name__ == "__main__":
    main()
