#!/usr/bin/env python3
"""
Abstract Monte Carlo for CLIENT_BASE household coverage (pre-implementation).

Illustrative probabilities — not calibrated RF measurements.
Goal: compare delivery-to-home and relative extra TX under policies.
"""
from __future__ import annotations

import random
from dataclasses import dataclass
from typing import Dict, List, Tuple

RNG = random.Random(42)

# --- Scenario knobs (urban attic / indoor home) ---
P_ROOF_HEARS_SRC = 0.92
P_HOME_HEARS_SRC = 0.18  # weak indoor
P_HOME_HEARS_DISTANT = 0.12  # distant mesh relay rarely reaches home
P_HOME_HEARS_ROOF = 0.88  # roof→home link OK
P_DISTANT_RELAY_EXISTS = 0.75  # someone else also floods
# CLIENT delay: chance roof hears distant dupe BEFORE its own TX → cancel
P_CANCEL_RACE_CLIENT = 0.55
P_CANCEL_RACE_LATE = 0.15  # late window: less often "already cancelled", still may TX after

N_PACKETS = 5000
N_FAVORITES = 3
TTL_HOURS = 24
MSG_PER_HOUR_SECONDARY = 4  # private channel TEXT rate at roof
POLL_HOURS_STAGGER = [0, 2, 26]  # Anna now, Bart +2h, Chris after TTL


@dataclass
class Outcome:
    home_got: bool
    roof_tx: bool
    cancelled: bool


def home_receives(heard_src: bool, heard_distant: bool, heard_roof: bool) -> bool:
    return heard_src or heard_distant or heard_roof


def sim_live(policy: str) -> Dict[str, float]:
    got = tx = cancel = 0
    for _ in range(N_PACKETS):
        roof_hears = RNG.random() < P_ROOF_HEARS_SRC
        home_src = RNG.random() < P_HOME_HEARS_SRC
        distant = RNG.random() < P_DISTANT_RELAY_EXISTS
        home_dist = distant and RNG.random() < P_HOME_HEARS_DISTANT

        roof_tx = False
        cancelled = False
        home_roof = False

        if roof_hears:
            if policy == "cb_today":
                # non-favorite broadcast → CLIENT cancel race
                if distant and RNG.random() < P_CANCEL_RACE_CLIENT:
                    cancelled = True
                else:
                    roof_tx = True
            elif policy == "l1_late":
                # never cancel for coverage; late TX still happens
                if distant and RNG.random() < P_CANCEL_RACE_LATE:
                    # "late" still transmits (ROUTER_LATE-like): race only delays, does not cancel
                    roof_tx = True
                else:
                    roof_tx = True
            elif policy == "router_late":
                roof_tx = True
            elif policy == "router":
                roof_tx = True  # early; same delivery model here (airtime differs elsewhere)
            else:
                raise ValueError(policy)

            if roof_tx and RNG.random() < P_HOME_HEARS_ROOF:
                home_roof = True

        if home_receives(home_src, home_dist, home_roof):
            got += 1
        if roof_tx:
            tx += 1
        if cancelled:
            cancel += 1

    return {
        "home_delivery_pct": 100.0 * got / N_PACKETS,
        "roof_tx_pct": 100.0 * tx / N_PACKETS,
        "cancel_pct": 100.0 * cancel / N_PACKETS,
    }


def sim_airtime_relative() -> List[Tuple[str, float]]:
    """
    Relative mesh-wide flood burden from ONE roof node for public+secondary mix.
    Units: arbitrary 'TX slots' per 100 mesh broadcasts heard by roof.
    """
    heard = 100
    # fraction of traffic that is private secondary vs all-channel flood
    private_frac = 0.15
    public_frac = 0.85

    # cb_today: TX only when not cancelled
    cb_tx = heard * (1.0 - P_DISTANT_RELAY_EXISTS * P_CANCEL_RACE_CLIENT) * P_ROOF_HEARS_SRC
    # l1: TX almost always when heard (scoped: model as all configured channels = same as heard)
    l1_tx = heard * P_ROOF_HEARS_SRC
    # router_late: same TX count as l1 for traffic it relays (entire mesh role)
    rl_tx = heard * P_ROOF_HEARS_SRC
    # router: same count, earlier (collision cost not modeled) — mark equal TX count
    r_tx = heard * P_ROOF_HEARS_SRC
    # l1+l2 hybrid mental model: L1 on all configured; if L1 only secondary:
    l1_sec_only = heard * private_frac * P_ROOF_HEARS_SRC + heard * public_frac * (
        1.0 - P_DISTANT_RELAY_EXISTS * P_CANCEL_RACE_CLIENT
    ) * P_ROOF_HEARS_SRC

    return [
        ("CLIENT_BASE today", cb_tx),
        ("L1 late all configured ch", l1_tx),
        ("L1 late secondary-only + CLIENT public", l1_sec_only),
        ("ROUTER_LATE (full role)", rl_tx),
        ("ROUTER (full role)", r_tx),
    ]


def sim_secretary_multi_reader() -> Dict[str, object]:
    """
    Shared ring + per-cursor vs purge-on-first-ACK over TTL window.
    """
    hours = TTL_HOURS + 6
    # messages arrive each hour
    ring: List[int] = []
    next_id = 0
    for h in range(hours):
        for _ in range(MSG_PER_HOUR_SECONDARY):
            ring.append(next_id)
            next_id += 1
        # TTL drop: remove older than TTL_HOURS
        # approximate: keep last TTL_HOURS * rate
        max_keep = TTL_HOURS * MSG_PER_HOUR_SECONDARY
        if len(ring) > max_keep:
            ring = ring[-max_keep:]

    def available_for_cursor(cursor: int) -> List[int]:
        return [m for m in ring if m >= cursor]

    # cursors start at 0
    results = {}
    # purge model: first pull empties
    purge_ring = list(ring)
    for i, t in enumerate(POLL_HOURS_STAGGER):
        name = ["Anna", "Bart", "Chris"][i]
        # rebuild ring state at poll time (simplified: use final ring for Anna/Bart within TTL;
        # Chris after TTL sees only recent window)
        if t <= TTL_HOURS:
            live = ring  # simplified end-state within window
        else:
            live = ring  # already TTL-trimmed to last 24h — Chris still gets recent, not ancient

        # cursor model
        cursor = 0 if i == 0 else (POLL_HOURS_STAGGER[0] * MSG_PER_HOUR_SECONDARY if i == 1 else 0)
        # Anna pulls all at T=0 from empty buffer build — re-sim timeline properly below
        results[f"{name}_cursor_note"] = "see timeline"

    # Proper mini-timeline
    timeline_msgs: List[Tuple[int, int]] = []  # (hour, msg_id)
    mid = 0
    for h in range(0, 30):
        for _ in range(MSG_PER_HOUR_SECONDARY):
            timeline_msgs.append((h, mid))
            mid += 1

    def msgs_in_window(at_hour: int) -> List[int]:
        lo = at_hour - TTL_HOURS
        return [m for h, m in timeline_msgs if lo < h <= at_hour]

    def pull_cursor(buf: List[int], cursor: int) -> Tuple[List[int], int]:
        batch = [m for m in buf if m >= cursor]
        new_c = (max(batch) + 1) if batch else cursor
        return batch, new_c

    def pull_purge(buf: List[int]) -> Tuple[List[int], List[int]]:
        batch = list(buf)
        return batch, []  # emptied

    cursors = {"Anna": 0, "Bart": 0, "Chris": 0}
    out = {}

    # Anna at h=0: buffer may be small — use h=5 for "now" after some traffic
    h_anna = 5
    buf = msgs_in_window(h_anna)
    a_batch, cursors["Anna"] = pull_cursor(buf, cursors["Anna"])
    out["Anna_T5_got"] = len(a_batch)
    out["Anna_cursor"] = cursors["Anna"]

    # Bart at h=7 (2h later): same shared buffer, own cursor
    h_bart = 7
    buf = msgs_in_window(h_bart)
    # Bart never pulled — cursor 0
    b_batch, cursors["Bart"] = pull_cursor(buf, 0)
    out["Bart_T7_got"] = len(b_batch)
    out["Bart_still_sees_Annas_msgs"] = len([m for m in b_batch if m < cursors["Anna"]]) > 0

    # Purge-on-ACK counterfactual: after Anna, buffer empty for Bart
    buf_p = msgs_in_window(h_anna)
    _, buf_p = pull_purge(buf_p)
    # new msgs hours 5..7
    new_only = [m for h, m in timeline_msgs if h_anna < h <= h_bart]
    out["Bart_if_purge_only_new"] = len(new_only)
    out["Bart_if_purge_misses_shared"] = len(a_batch)

    # Chris at h=26: TTL 24h from then → window (2,26]
    h_chris = 26
    buf = msgs_in_window(h_chris)
    c_batch, cursors["Chris"] = pull_cursor(buf, 0)
    out["Chris_T26_got"] = len(c_batch)
    out["Chris_misses_pre_TTL"] = len([m for h, m in timeline_msgs if h <= h_chris - TTL_HOURS])
    out["L2_unicast_TX_Anna"] = out["Anna_T5_got"]  # 1 TX per msg stop-and-wait abstract
    out["L2_unicast_TX_Bart"] = out["Bart_T7_got"]
    out["L2_vs_flood_100_neighbors"] = "unicast to N favorites only — not 100-node flood"

    # airtime rough: each unicast ~1 packet; flood rebroadcast ~1 per hearing node role
    out["L2_airtime_slots_two_pulls"] = out["Anna_T5_got"] + out["Bart_T7_got"]
    return out


def main() -> None:
    print("=== PLAN (phases) ===")
    print("P0 Abstract sim (this script)")
    print("P1 Lock L1 policy from sim conclusions")
    print("P2 L1 firmware + Unity")
    print("P3 L2 storage spike + protocol")
    print("P4 L2 MVP + tests")
    print("P5 Docs/PR; L3 later")
    print()

    print("=== LIVE DELIVERY (Monte Carlo, n=%d) ===" % N_PACKETS)
    rows = []
    for policy, label in [
        ("cb_today", "CLIENT_BASE today"),
        ("l1_late", "L1 late never-cancel"),
        ("router_late", "ROUTER_LATE"),
        ("router", "ROUTER"),
    ]:
        r = sim_live(policy)
        rows.append((label, r))
        print(
            f"{label:28s}  home_delivery={r['home_delivery_pct']:5.1f}%  "
            f"roof_tx={r['roof_tx_pct']:5.1f}%  cancel={r['cancel_pct']:5.1f}%"
        )

    print()
    print("=== RELATIVE ROOF TX / 100 HEARD (airtime proxy) ===")
    for name, val in sim_airtime_relative():
        print(f"{name:42s}  {val:5.1f} TX slots")

    print()
    print("=== SECRETARY MULTI-READER ===")
    for k, v in sim_secretary_multi_reader().items():
        print(f"  {k}: {v}")

    print()
    print("=== PRELIMINARY CONCLUSIONS ===")
    cb = rows[0][1]["home_delivery_pct"]
    l1 = rows[1][1]["home_delivery_pct"]
    print(f"1) L1 lifts home delivery ~{cb:.0f}% → ~{l1:.0f}% in this hidden-terminal model.")
    print("2) L1 roof_tx ≈ ROUTER_LATE count for traffic it elects to relay — scope must be narrow.")
    print("3) L1 secondary-only keeps public closer to CLIENT today; private gets reliability.")
    print("4) Cursor model: Bart still gets Anna's msgs; purge-on-ACK makes Bart miss shared history.")
    print("5) After TTL, Chris only gets recent window — L3 or longer TTL needed for latecomers.")
    print("6) L2 airtime scales with favorites×unread, not mesh size — safer than city ROUTER.")


if __name__ == "__main__":
    main()
