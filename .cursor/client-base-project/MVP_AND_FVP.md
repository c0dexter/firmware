# MVP vs FVP - CLIENT_BASE household base

_Last updated: 2026-10-21_

**MVP** = Minimum Viable Product: smallest shippable slice that fixes the core lie of the role and is safe for the mesh.  
**FVP** = Full Viable Product (pełna wizja produktu): kompletny „dach jako sekretarka gospodarstwa”, w tym catch-up poza TTL i wygodny UX - bez zamiany bazy w miejski ROUTER.

Policy lock: `ANALYSIS_AND_SIMULATION.md`, `README.md`.

---

## MVP - co dokładnie rozumiemy

### Cel MVP

1. Domownik na private secondary **dostaje na żywo** to, co dach usłyszał (koniec typowego „telefon na dachu widzi, radio w domu nie”).
2. Jak dom był offline / przegapił okno - może **ściągnąć** z dachu historię secondary (pull), bez kasowania jej innym domownikom.
3. Domyślnie **nie** zwiększamy publicznego airtime jak ROUTER_LATE.

### W zakresie MVP

| ID  | Element              | Uwagi                                                                                                               |
| --- | -------------------- | ------------------------------------------------------------------------------------------------------------------- |
| M1  | **L1 default**       | Late never-cancel na **secondary obecnych na bazie**                                                                |
| M2  | **L1 opt-in config** | Elevate **primary TEXT** - default **off** (może wejść w MVP jako flaga, nawet jeśli docs mówią „używaj świadomie”) |
| M3  | **L2 sekretarka**    | Bufor TEXT na tych secondary; shared ring; **cursor per favorite**; TTL; limity `history_return_max`                |
| M4  | **Platformy**        | ESP (RAM/PSRAM) + nRF (flash+index); lean rekord ~256 B; **STM32WL bez L2**                                         |
| M5  | **ACL**              | Tylko favorites (= served) mogą pollować / być celem serwisu                                                        |
| M6  | **Testy**            | Unity: L1 cancel/relay na secondary; L2 cursory / TTL / no purge-on-first                                           |
| M7  | **Docs**             | Ten folder + opis w PR; uczciwy UX: primary bez opt-in = best-effort                                                |
| M8  | **Perf**             | Brak I/O flash na ścieżce RX cancel; cap favorites + dump size                                                      |

### Poza MVP (świadomie)

- L3: sync historii po TTL (LoRa peer albo **BT między telefonami**).
- Pełna elevacja primary / wszystkich portnumów.
- Zamiana / fork globalnego `StoreForwardModule`.
- Nowa rola w enum.
- InkHUD / pełny polish UI wszystkich klientów (wystarczy protokół + jeden tor testowy; app polish może iść równolegle).
- Idealne pomiary RF w terenie (symulacje wystarczą do locka; field test = walidacja po MVP).

### Kryteria „MVP done”

- [x] UC1 (secondary live) przechodzi testami Unity (`test_client_base_l1`) - hardware smoke still open.
- [x] UC5/UC6 store-level (multi-cursor + TTL) - `test_client_base_l2_store`; poll/unicast path still open.
- [ ] UC2 uczciwie udokumentowane (primary bez opt-in).
- [x] UC8: default nie zachowuje się jak ROUTER na public (primary non-favorite still cancels).
- [ ] nRF i co najmniej jeden ESP budują się z L2 caps.
- [ ] PR + knowledge plane aktualne; `trunk fmt`; native tests green dla nowych suite’ów.

### Kolejność implementacji MVP

1. **L1 + Unity** - **DONE** (2026-10-21).
2. **L2 storage (ring/cursors)** - **DONE** store core; **module + poll/unicast** - **PAUSED** (resume next).
3. **Opt-in primary TEXT** (config) jeśli nie blokuje 1-2.
4. Hardening caps / docs / field smoke.

---

## FVP - pełna wizja (po MVP)

FVP = MVP **plus** domknięcie obietnicy „sekretarka gospodarstwa” end-to-end.

### FVP - kolejne kroki (proponowana kolejność)

| Krok   | Nazwa                     | Opis                                                                                                 |
| ------ | ------------------------- | ---------------------------------------------------------------------------------------------------- |
| **F1** | Primary UX complete       | Opt-in primary TEXT dopracowany w appach; jasne ostrzeżenie airtime; ewentualnie presets „home base” |
| **F2** | Client protocol polish    | Oficjalne protobuf + Android/iOS/web: poll, progress, „unread on base”, błędy TTL                    |
| **F3** | Storage hardening         | Per-MEM_CLASS tuning z pomiarów; wear-leveling na nRF; telemetry „buffer % / oldest msg”             |
| **F4** | L3a - phone BT/Wi‑Fi sync | Domownicy wymieniają **już pobrane** private msgs między **apkami** (nie LoRa) - po TTL dachu        |
| **F5** | L3b - optional radio peer | Favorite↔favorite inventory+pull po LoRa tylko jeśli F4 nie wystarcza (radią bez telefonu)           |
| **F6** | Ops / observability       | Logi/metrik: cancel avoided, secretary hits, dump duration; docs upstream (meshtastic/meshtastic)    |
| **F7** | Field validation          | Scenariusze miejskie vs rural; porównanie airtime vs ROUTER escape hatch                             |

### Kryteria „FVP done”

- Domownik wracający po TTL nadal ma ścieżkę odzysku (F4 i/lub F5).
- Appki prowadzą użytkownika (favorites, secondary na bazie, opt-in primary) bez forów „ustaw ROUTER”.
- Upstream docs + ewentualny merge path; brak regresji meshu w field testach.
- L2 na wspieranych klasach pamięci stabilne długoterminowo (flash wear, caps).

---

## Jednym rzutem oka

```
MVP:  L1 secondary live + L2 pull/cursors/TTL + caps + tests + honest primary
FVP:  MVP + app UX + hardening + L3 phone sync (+ optional LoRa peer) + upstream docs/field
```

**Następny konkretny krok teraz (start MVP):** Unity testy opisujące dzisiejsze vs pożądane L1 na secondary, potem implementacja L1.
