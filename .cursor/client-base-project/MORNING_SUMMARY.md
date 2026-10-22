# Morning summary - overnight desk session (2026-10-22)

_Session window ~00:33-… CEST · Branch `feature/client-base-role-improvements` · Tip before fixes: `fa6daed0a`_

## Verdict

| Area                    | Result                                                                |
| ----------------------- | --------------------------------------------------------------------- |
| **L2 poll dump bug**    | **Found + fixed** - dump thread never re-armed after ctor `disable()` |
| **L2 E2E header**       | **PASS** on desk after flash (`ROUTER_HISTORY`)                       |
| **Fake NodeDB stress**  | **PASS** (no reboot): 45→82 nodes, +60 contacts, rebootCount stayed 5 |
| **L1.5 FreeHop log**    | Still not CDC-proven (protobuf mode); precondition still PASS         |
| **Unity L2 poll+store** | **14/14 PASSED** (incl. new skip-own Empty case)                      |

## Bugs found and fixed (uncommitted)

1. **Dump OSThread never enabled** (`ClientBaseSecretaryModule.cpp`)  
   Ctor calls `disable()` (`enabled=false`). `Started` only called `setIntervalFromNow`, which does **not** set `enabled=true`. Payload dump never ran.  
   Fix: `enabled = true` before `setIntervalFromNow` on `Started`.

2. **Requester’s own TEXTs counted as dumpable history**  
   Desk 2-node seeds are home→roof; dump forged `from=home`→`to=home`, so home never showed TEXT. Classic S&F skips `from==dest`.  
   Fix: `availableCountNotFrom` + beginPoll uses it; `sendNextPayload` skips `from==dest`. Unity: `test_empty_when_only_requester_own_texts`.

## Hardware evidence

| Check                      | Evidence                                                                             |
| -------------------------- | ------------------------------------------------------------------------------------ |
| L2 header after enable fix | `ROUTER_HISTORY history_messages: 2` home←roof                                       |
| Fat NodeDB + L2            | `nodedbCount=82`, still `ROUTER_HISTORY` PASS, `rebootCount=5`                       |
| Fake contacts              | `scripts/fake_nodedb_stress.py`: 60× `add_contact`, 10 favorites, 10 secondary seeds |
| Home still favorite        | CLI nodes table shows `!ebba7ec6` Fav `*`                                            |

## Still open

- L1.5: observe `hop_start++` (noproto / 3rd sniffer / field).
- L2 dump TEXT over air for **third-party** `from` (needs 3rd node or frame inject).
- RF3 roof→home secondary still flaky in harness.
- Commit ritual when you return (enable fix + skip-own + stress script + docs).

## Artifacts

- `/tmp/cb_desk_smoke/l2_poll_retest.txt` - first PASS header
- `/tmp/cb_fake_nodedb/` - stress events; CLI confirmed nodedb 82 / reboot 5
- Scripts: `scripts/fake_nodedb_stress.py`, `desk_smoke.py` L2a now FAIL if silent

## Next when you wake

1. `git status` / commit L2 enable + skip-own + docs (ask agent).
2. Confirm Unity: `./bin/run-tests.sh -f test_client_base_l2_poll -f test_client_base_l2_store` alone.
3. Optional: 3rd node or inject for dump TEXT proof; noproto for FreeHopBumpStart.
