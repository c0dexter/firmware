#!/usr/bin/env python3
"""Prove CLIENT_BASE L1.5 FreeHopBumpStart on desk (2 nodes).

Requires roof: role CLIENT_BASE, home favorited, security.debug_log_api_enabled=true.
Listens meshtastic.log.line for 'CLIENT_BASE L1.5 free hop'.

Usage:
  python3 l15_freehop_proof.py --roof /dev/ttyACM3 --home /dev/ttyACM2
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import time

from meshtastic.serial_interface import SerialInterface
from pubsub import pub

OUT = "/tmp/cb_l15_proof"
os.makedirs(OUT, exist_ok=True)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--roof", required=True)
    ap.add_argument("--home", required=True)
    ap.add_argument("--rounds", type=int, default=3)
    args = ap.parse_args()

    roof = SerialInterface(args.roof)
    home = SerialInterface(args.home)
    time.sleep(2)
    home_id = f"!{home.localNode.nodeNum:08x}"
    try:
        roof.localNode.setFavorite(home_id)
    except Exception as e:
        print("fav_err", e)

    logs: list[str] = []
    rx: list[dict] = []

    def on_log(line, interface=None):
        logs.append(line)
        if re.search(r"L1\.5 free hop|FreeHop", line, re.I):
            print("LOGHIT", line, flush=True)

    def on_rx(packet, interface=None):
        if interface is not roof:
            return
        d = packet.get("decoded") or {}
        text = d.get("text")
        if text and str(text).startswith("l15p-"):
            row = {
                "fromId": packet.get("fromId"),
                "hopStart": packet.get("hopStart"),
                "hopLimit": packet.get("hopLimit"),
                "text": text,
            }
            rx.append(row)
            print("ROOF_RX", row, flush=True)

    pub.subscribe(on_log, "meshtastic.log.line")
    pub.subscribe(on_rx, "meshtastic.receive")

    for ch in (1, 0):
        for i in range(args.rounds):
            msg = f"l15p-ch{ch}-{i}-{int(time.time())}"
            print("TX", msg, flush=True)
            home.sendText(msg, channelIndex=ch, wantAck=False)
            time.sleep(2.2)
    time.sleep(4)

    hits = [l for l in logs if re.search(r"L1\.5 free hop", l, re.I)]
    pre = any(
        r.get("hopStart") == r.get("hopLimit") and r.get("hopStart") is not None
        for r in rx
    )
    verdict = "PASS" if hits else ("PRECOND_ONLY" if pre else "FAIL")
    summary = {"verdict": verdict, "hits": hits[:20], "rx": rx, "n_logs": len(logs)}
    with open(f"{OUT}/summary.json", "w") as f:
        json.dump(summary, f, indent=2)
    print("VERDICT", verdict, flush=True)
    home.close()
    roof.close()
    return 0 if verdict == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
