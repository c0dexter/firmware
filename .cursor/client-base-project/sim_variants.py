#!/usr/bin/env python3
"""
Multi-variant abstract simulation for CLIENT_BASE policy choice.
Illustrative — used for design confidence, not RF certification.
"""
from __future__ import annotations

import random
from dataclasses import dataclass
from typing import Dict, List, Tuple

N = 8000


@dataclass(frozen=True)
class World:
    name: str
    p_roof_src: float
    p_home_src: float
    p_home_distant: float
    p_home_roof: float
    p_distant: float
    p_cancel_client: float
    private_frac: float  # share of heard traffic that is private secondary


WORLDS = [
    World("urban_hidden", 0.92, 0.18, 0.12, 0.88, 0.78, 0.58, 0.18),
    World("suburban", 0.90, 0.35, 0.25, 0.90, 0.55, 0.45, 0.15),
    World("quiet_rural", 0.95, 0.55, 0.40, 0.92, 0.25, 0.30, 0.12),
    World("dense_city_harsh", 0.88, 0.10, 0.08, 0.80, 0.90, 0.70, 0.20),
]


# Policies
POLICIES = [
    "cb_today",
    "l1_all_channels",
    "l1_secondary_only",
    "l1_secondary_plus_primary_text",  # secondary full; primary only TEXT port elevates
    "l1_when_has_favorites_all_ch",  # same as l1_all if favorites exist (we assume they do)
    "router_late",
    "router",
]


def is_private(rng: random.Random, private_frac: float) -> bool:
    return rng.random() < private_frac


def is_primary_text(rng: random.Random) -> bool:
    # of public traffic, ~40% TEXT (rest nodeinfo/pos/telemetry noise)
    return rng.random() < 0.40


def run_live(world: World, policy: str, seed: int) -> Dict[str, float]:
    rng = random.Random(seed)
    got = tx = cancel = 0
    # mesh harm proxy: TX that is NOT needed for household private (public flood TX)
    public_tx = 0
    private_tx = 0

    for _ in range(N):
        private = is_private(rng, world.private_frac)
        primary_text = (not private) and is_primary_text(rng)

        roof = rng.random() < world.p_roof_src
        home_src = rng.random() < world.p_home_src
        distant = rng.random() < world.p_distant
        home_dist = distant and rng.random() < world.p_home_distant

        elevate = False
        if policy == "cb_today":
            elevate = False  # non-fav broadcast
        elif policy in ("l1_all_channels", "l1_when_has_favorites_all_ch", "router_late", "router"):
            elevate = True
        elif policy == "l1_secondary_only":
            elevate = private
        elif policy == "l1_secondary_plus_primary_text":
            elevate = private or primary_text
        else:
            raise ValueError(policy)

        roof_tx = False
        cancelled = False
        home_roof = False

        if roof:
            if elevate:
                roof_tx = True
            else:
                if distant and rng.random() < world.p_cancel_client:
                    cancelled = True
                else:
                    roof_tx = True
            if roof_tx and rng.random() < world.p_home_roof:
                home_roof = True

        if home_src or home_dist or home_roof:
            got += 1
        if roof_tx:
            tx += 1
            if private:
                private_tx += 1
            else:
                public_tx += 1
        if cancelled:
            cancel += 1

    return {
        "home_pct": 100.0 * got / N,
        "roof_tx_pct": 100.0 * tx / N,
        "cancel_pct": 100.0 * cancel / N,
        "public_tx_pct": 100.0 * public_tx / N,
        "private_tx_pct": 100.0 * private_tx / N,
    }


def score(world: World, r: Dict[str, float]) -> Tuple[float, str]:
    """
    Higher is better for the *system*:
    reward home delivery, penalize public mesh TX (city airtime), mild penalty all TX.
    """
    # weights: household reliability vs not becoming another city router
    s = 2.0 * r["home_pct"] - 1.4 * r["public_tx_pct"] - 0.3 * r["roof_tx_pct"]
    return s, f"2*home - 1.4*public_tx - 0.3*all_tx = {s:.1f}"


def secretary_offline_case() -> Dict[str, float]:
    """
    Home offline 6h, then returns. L1 alone vs L1+L2 cursor pull.
    """
    rate = 4  # msg/h secondary
    offline_h = 6
    buffered = rate * offline_h
    # L1: home heard none while offline
    l1_only_got = 0
    # L1+L2: pull all buffered (unicast cost = buffered)
    l2_got = buffered
    # ROUTER while offline also 0 to home if home radio off
    return {
        "offline_hours": offline_h,
        "msgs_missed_without_L2": buffered,
        "L1_only_recovery": l1_only_got,
        "L1_plus_L2_recovery": l2_got,
        "L2_unicast_cost": buffered,
        "city_flood_cost_if_was_ROUTER": "N/A continuous; L2 is burst local only",
    }


def main() -> None:
    print("=== SYSTEM SCORE (higher = better for mesh + household) ===\n")
    # aggregate ranks
    rank_sum: Dict[str, float] = {p: 0.0 for p in POLICIES}

    for wi, world in enumerate(WORLDS):
        print(f"--- World: {world.name} ---")
        print(
            f"{'policy':40s} {'home%':>7} {'tx%':>7} {'pubTX%':>7} {'privTX%':>8} {'score':>8}"
        )
        best_s = -1e9
        best_p = ""
        for pi, policy in enumerate(POLICIES):
            r = run_live(world, policy, seed=1000 + wi * 17 + pi)
            s, _ = score(world, r)
            rank_sum[policy] += s
            if s > best_s:
                best_s, best_p = s, policy
            print(
                f"{policy:40s} {r['home_pct']:7.1f} {r['roof_tx_pct']:7.1f} "
                f"{r['public_tx_pct']:7.1f} {r['private_tx_pct']:8.1f} {s:8.1f}"
            )
        print(f"  >> best in this world: {best_p} ({best_s:.1f})\n")

    print("=== AGGREGATE SCORE ACROSS WORLDS ===")
    ranked = sorted(rank_sum.items(), key=lambda x: -x[1])
    for p, s in ranked:
        print(f"  {p:40s}  {s:8.1f}")

    print("\n=== OFFLINE HOME (L2 necessity) ===")
    for k, v in secretary_offline_case().items():
        print(f"  {k}: {v}")

    print("\n=== ADVICE SUMMARY ===")
    winner = ranked[0][0]
    print(f"Aggregate winner: {winner}")
    print(
        "Prefer scoped L1 (secondary / secondary+primary TEXT) over L1-all or full ROUTER_LATE "
        "when optimizing household delivery WITHOUT city-scale public TX."
    )


if __name__ == "__main__":
    main()
