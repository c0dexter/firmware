#!/usr/bin/env python3
"""Fill CLIENT_BASE NodeDB with fictitious contacts + favorite pressure.

Goal: memory/eviction load while L1/L2 paths stay alive (desk overnight case).

Uses AdminMessage.add_contact to insert SharedContact stubs (creates NodeDB
entries via getOrCreateMeshNode). Then favorites as many as the protected
cap allows, floods secondary TEXT from home, polls L2, watches rebootCount.

Usage:
  python3 fake_nodedb_stress.py --roof /dev/ttyACM3 --home /dev/ttyACM2 [--count 80]
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
from datetime import datetime, timezone

from meshtastic.protobuf import admin_pb2, mesh_pb2, portnums_pb2, storeforward_pb2
from meshtastic.serial_interface import SerialInterface
from pubsub import pub

OUT = "/tmp/cb_fake_nodedb"
os.makedirs(OUT, exist_ok=True)


def ts() -> str:
    return datetime.now(timezone.utc).isoformat()


def log(msg: str, **kw):
    row = {"t": ts(), "msg": msg, **kw}
    print(json.dumps(row, ensure_ascii=False), flush=True)
    with open(f"{OUT}/events.jsonl", "a") as f:
        f.write(json.dumps(row, ensure_ascii=False) + "\n")


def node_id(n: int) -> str:
    return f"!{n:08x}"


def make_contact(num: int, i: int) -> admin_pb2.SharedContact:
    c = admin_pb2.SharedContact()
    c.node_num = num
    c.user.CopyFrom(mesh_pb2.User())
    c.user.id = node_id(num)
    c.user.long_name = f"Fake{i:03d}"
    # short_name max 4 chars
    c.user.short_name = f"F{i % 100:02d}"
    c.user.hw_model = mesh_pb2.HardwareModel.UNSET
    # Deterministic fake 32-byte pubkey so PKI slots look "real" to eviction.
    key = bytes((i + j) % 256 for j in range(32))
    c.user.public_key = key
    return c


def myinfo(iface: SerialInterface) -> dict:
    """Pull reboot/nodedb from MyNodeInfo (protobuf attrs or dict)."""
    mi = getattr(iface, "myInfo", None)
    num = iface.localNode.nodeNum
    reboot = None
    ndb = None
    if mi is not None:
        reboot = getattr(mi, "reboot_count", None)
        if reboot is None and isinstance(mi, dict):
            reboot = mi.get("rebootCount") or mi.get("reboot_count")
        ndb = getattr(mi, "nodedb_count", None)
        if ndb is None and isinstance(mi, dict):
            ndb = mi.get("nodedbCount") or mi.get("nodedb_count")
        n = getattr(mi, "my_node_num", None)
        if n:
            num = n
    # Fallback: count local nodes dict (client-side view)
    if ndb is None and getattr(iface, "nodes", None) is not None:
        ndb = len(iface.nodes)
    return {"num": num, "rebootCount": reboot, "nodedbCount": ndb}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--roof", required=True)
    ap.add_argument("--home", required=True)
    ap.add_argument("--count", type=int, default=80, help="fictitious contacts to add")
    ap.add_argument(
        "--favorite", type=int, default=12, help="how many fakes to favorite"
    )
    ap.add_argument(
        "--seed-msgs", type=int, default=15, help="secondary TEXT from home"
    )
    args = ap.parse_args()

    open(f"{OUT}/events.jsonl", "w").close()
    roof = SerialInterface(args.roof)
    home = SerialInterface(args.home)
    time.sleep(1.5)

    roof_id = node_id(roof.localNode.nodeNum)
    home_id = node_id(home.localNode.nodeNum)
    base = myinfo(roof)
    log("baseline", roof=base, home=myinfo(home), roof_id=roof_id, home_id=home_id)

    # Keep real home favorite first
    try:
        roof.localNode.setFavorite(home_id)
    except Exception as e:
        log("setFavorite_home_err", err=str(e))

    # Base fake node nums in a high unused range
    fake_base = 0xA5000000
    added = 0
    fav_ok = 0
    for i in range(args.count):
        num = fake_base + i
        contact = make_contact(num, i)
        try:
            p = admin_pb2.AdminMessage()
            p.add_contact.CopyFrom(contact)
            roof.localNode._sendAdmin(p)
            added += 1
        except Exception as e:
            log("add_contact_fail", i=i, err=str(e))
            break
        if i < args.favorite:
            try:
                roof.localNode.setFavorite(node_id(num))
                fav_ok += 1
            except Exception as e:
                log("favorite_fail", i=i, err=str(e))
        if (i + 1) % 20 == 0:
            mid = myinfo(roof)
            log("progress", i=i + 1, added=added, fav_ok=fav_ok, roof=mid)
            if base.get("rebootCount") is not None and mid.get(
                "rebootCount"
            ) != base.get("rebootCount"):
                log("REBOOT_DURING_FILL", before=base, after=mid)
                home.close()
                roof.close()
                return 2
        time.sleep(0.15)

    after_fill = myinfo(roof)
    log("after_fill", added=added, fav_ok=fav_ok, roof=after_fill)

    # Secondary TEXT flood (home → roof) to exercise L2 ingest under fat NodeDB
    rx = []

    def on_rx(packet, interface):
        if interface is home:
            d = packet.get("decoded") or {}
            rx.append(
                {
                    "fromId": packet.get("fromId"),
                    "port": d.get("portnum"),
                    "text": d.get("text"),
                }
            )

    pub.subscribe(on_rx, "meshtastic.receive")
    seed_ok = 0
    for i in range(args.seed_msgs):
        try:
            home.sendText(
                f"fake-stress-{i}-{int(time.time())}", channelIndex=1, wantAck=False
            )
            seed_ok += 1
        except Exception as e:
            log("seed_fail", i=i, err=str(e))
        time.sleep(0.8)
    time.sleep(5)
    log("seed_done", seed_ok=seed_ok)

    # L2 poll - expect at least ROUTER_HISTORY (empty or not)
    before = len(rx)
    sf = storeforward_pb2.StoreAndForward()
    sf.rr = storeforward_pb2.StoreAndForward.CLIENT_HISTORY
    sf.history.window = 0
    try:
        home.sendData(
            sf.SerializeToString(),
            destinationId=roof_id,
            portNum=portnums_pb2.PortNum.STORE_FORWARD_APP,
            wantAck=False,
            channelIndex=0,
        )
    except Exception as e:
        log("poll_send_fail", err=str(e))
    time.sleep(20)
    replies = [p for p in rx[before:] if p.get("fromId") == roof_id]
    log("l2_poll", replies=replies[:10], n=len(replies))

    final = myinfo(roof)
    reboot_delta = None
    if base.get("rebootCount") is not None and final.get("rebootCount") is not None:
        reboot_delta = final["rebootCount"] - base["rebootCount"]
    summary = {
        "baseline": base,
        "after_fill": after_fill,
        "final": final,
        "added": added,
        "fav_ok": fav_ok,
        "seed_ok": seed_ok,
        "l2_replies": len(replies),
        "reboot_delta": reboot_delta,
        "verdict": (
            "PASS"
            if reboot_delta == 0 and added > 0
            else ("WARN" if reboot_delta == 0 else "FAIL")
        ),
    }
    with open(f"{OUT}/summary.json", "w") as f:
        json.dump(summary, f, indent=2)
    log("summary", **summary)

    # Restore: unfavorite fakes so ACL stays home-only for later desk smoke
    for i in range(min(args.favorite, added)):
        try:
            roof.localNode.removeFavorite(node_id(fake_base + i))
        except Exception:
            pass
    try:
        roof.localNode.setFavorite(home_id)
    except Exception:
        pass

    home.close()
    roof.close()
    return 0 if summary["verdict"] != "FAIL" else 1


if __name__ == "__main__":
    sys.exit(main())
