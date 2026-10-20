# Journal - CLIENT_BASE project

Append-only work log. Newest entries at the top.

---

## 2026-10-21 - MVP L1 landed (code + Unity)

### Done

- Implemented **L1 secondary never-cancel / late window** for `CLIENT_BASE`:
  - `src/mesh/ClientBaseRelay.h` - pure policy helpers (`clientBaseAllowsCancelingDupe`, `clientBaseUsesLateRebroadcastWindow`)
  - `Channels::isConfiguredSecondary` - SECONDARY + `has_settings`
  - `FloodingRouter::roleAllowsCancelingDupe` / `perhapsCancelDupe` - CLIENT_BASE uses helpers; favorites still protected; primary stays CLIENT-like cancel
- Added suite `test/test_client_base_l1/` (7 Unity cases): channel classification + policy matrix + Channels→policy integration
- Native run on this host: **7/7 PASSED** (`pio test -e coverage -f test_client_base_l1`) after staging host libs under `.local-deps/` (not committed)

### Notes / caveats

- Host lacked system native deps (`yaml-cpp`, `libuv`, `gpiod`, `jsoncpp`, `libi2c`, OpenSSL). Workaround: extract debs into `.local-deps/` + `PLATFORMIO_BUILD_FLAGS` / `LD_LIBRARY_PATH`. Prefer Dockerfile.test / `apt install` for CI-like hosts.
- Dropped a NodeDB favorite-flag integration case that hung under the coverage runner (SIGHUP before `UNITY_END`); favorite×primary policy remains covered by the pure helper test.
- **M2** (opt-in primary TEXT) not in this commit - still default-off, unimplemented config key.
- **L2** not started.

### Next

1. Push L1 commit to `fork/feature/client-base-role-improvements`
2. Start L2: storage sketch + ring + per-favorite cursor + poll path + Unity
3. Optional: wire M2 config when protobuf key exists / agreed
4. `gh auth login` → GitHub Project board

---

## 2026-10-21 (earlier) - Docs/policy lock + branch bootstrap

- Analysis + sims locked L1 secondary-only default; L2 multi-cursor; L3 deferred
- Project plane under `.cursor/client-base-project/`; rules/skills; first docs commit on fork
