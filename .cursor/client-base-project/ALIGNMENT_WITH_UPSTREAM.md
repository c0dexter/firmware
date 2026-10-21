# Alignment: nasze propozycje vs upstream (czy nas odrzucą?)

_Last updated: 2026-10-21_

Cel: mieć pewność, że idziemy **z** społecznością / maintainerami, a nie pod prąd.  
Źródła: locked policy (`ANALYSIS_AND_SIMULATION.md`), nasze L1/L2, issue/PR: #12156, #12180, #10704, #9942, #8347, komentarze (TEXT-only, airtime).

---

## 1. Wspólny punkt wyjścia (tu na pewno nie jesteśmy „obcy”)

Wszyscy opisują **ten sam bug doświadczenia**:

> Dach `CLIENT_BASE` + favorites **nie** dostarcza indoorowi broadcastów kanału, bo cancel-on-dupe + `to=broadcast` nie widzi „dla favorite”.

To nie jest nasz wymysł - to #7863 (historia roli), #8820, #8366, #10704, **#12156**, field numbers w #12180 (~19% RX companion vs ~90% przy ROUTER_LATE).

**Maintainerzy boją się jednego:** attic = stealth **ROUTER** na LongFast → miasto umiera.  
Nasza polityka i nasze symulacje **adresują dokładnie ten strach**.

---

## 2. Tabela: pomysł → kto → zgodność z nami → ryzyko odrzucenia

### A. Live delivery (L1) - „dach ma faktycznie nadać”

| Pomysł                                          | Kto                                                    | Co robi                                                                    | Zgodność z nami                                  | Ryzyko odrzucenia _naszego_ wariantu                                                                          |
| ----------------------------------------------- | ------------------------------------------------------ | -------------------------------------------------------------------------- | ------------------------------------------------ | ------------------------------------------------------------------------------------------------------------- |
| **Secondary never-cancel + late**               | **My (locked L1)**                                     | Chroni prywatne kanały na bazie; primary bez opt-in = CLIENT               | -                                                | **Niskie-średnie** - idzie _z_ argumentem airtime; może być „za wąskie” dla ludzi na samym LongFast           |
| **Fill-in gdy direct favorite**                 | **#12180** (open → ~2.8.3)                             | Wszystkie broadcasty: late zamiast cancel, chyba że favorite sam relayował | Ten sam problem; **szerszy TX** (także public)   | Ich PR może wygrać „szybszy fix na public”; nasze L1 nadal wartościowe na secondary / bez obecności hops_away |
| **0-hop late fill-in ± power backoff**          | #12156 (propozycja)                                    | Terminal delivery, mniej spillover                                         | Komplementarne do live; my tego nie robimy w MVP | Niskie dla _idei_; implementacja power/hop_start = review risk                                                |
| **TEXT-only elevate**                           | komentarze #12156 / #10704                             | Tylko TEXT (i kanały które dach zna)                                       | Blisko **M2** + „TEXT-focused L2”                | **Niskie** - maintainer-friendly (mało % ruchu)                                                               |
| **Always fill-in / L1-all / attic=ROUTER_LATE** | odrzucane w dyskusjach; my **zabraniamy** jako default | Pełny public amplify                                                       | Świadomie **nie**                                | Gdybyśmy to zaproponowali jako default → **wysokie** odrzucenie                                               |

### B. Po fakcie / offline (L2) - „ściągnij z dachu”

| Pomysł                                                 | Kto                             | Co robi                  | Zgodność                                          | Ryzyko                                                          |
| ------------------------------------------------------ | ------------------------------- | ------------------------ | ------------------------------------------------- | --------------------------------------------------------------- |
| **Pull / S&F-like z dachu, 0-hop answer**              | **#10704** (stale) + komentarze | Indoor prosi o brakujące | = **nasze L2**                                    | Średnie na _protokół_ (S&F vs nowe msg), **niskie na potrzebę** |
| **Shared ring + per-favorite cursor, no purge-on-ACK** | **My**                          | Multi-occupant home      | Brak sprzecznego PR; #10704 nie precyzuje cursors | Niskie jeśli caps + nie globalny PSRAM S&F                      |
| **Global StoreForward na primary**                     | utrzymanie świadomie ogranicza  | Inny produkt             | My **nie** to robimy                              | Wysokie gdybyśmy forkowali global S&F                           |

### C. Rola / produkt

| Pomysł                              | Kto                                       | Zgodność                | Ryzyko                                                                   |
| ----------------------------------- | ----------------------------------------- | ----------------------- | ------------------------------------------------------------------------ |
| Zostaw `CLIENT_BASE`, nie nowy enum | **My** + **#9942** (zostawia Client_base) | Silna                   | Niskie                                                                   |
| Rename UX „Home”                    | komentarze #9942                          | OK później              | Niskie                                                                   |
| Gate ROUTER behind admin            | #8347 (2.9)                               | Nie atakuje CLIENT_BASE | Wręcz **wspiera** sens poprawiania CLIENT_BASE zamiast ucieczki w ROUTER |

### D. Hops „za darmo” (#8366)

Poza naszym MVP. Nie koliduje; nie opierać na tym PR L1/L2.

---

## 3. Gdzie jesteśmy „bardziej konserwatywni” niż #12180 (to plus u review)

|                                 | My                                       | #12180                                                                               |
| ------------------------------- | ---------------------------------------- | ------------------------------------------------------------------------------------ |
| Public LongFast default         | **Nie** elevates (bez M2)                | Elevates gdy jest direct favorite                                                    |
| Prywatny secondary              | Elevates zawsze (kanał na bazie)         | Elevates tylko w ramach „broadcast + favorite nearby” (też działa, ale inny trigger) |
| Argument airtime                | Sim: secondary-only wygrywa system score | Field: duży zysk delivery, nadal < ROUTER_LATE                                       |
| Zależność od NodeDB `hops_away` | Nie (kanał)                              | Tak (presence) - krytyka w #12156 (CLIENT_MUTE cichy)                                |

**Maintainer narrative, który przejdzie:**  
„Nie robimy attic ROUTER. Robimy **scoped** reliability: private channels + opcjonalnie TEXT + local pull. Public zostaje świadomie best-effort albo osobny opt-in.”

To jest **dokładnie** język z #12156 („maintainers fear recreating ROUTER”).

---

## 4. Scenariusze odrzucenia - uczciwie

| Gdybyśmy…                                                   | Prawdopodobieństwo odrzucenia                       |
| ----------------------------------------------------------- | --------------------------------------------------- |
| PR: „CLIENT_BASE = never cancel everything”                 | **Wysokie**                                         |
| PR: secondary never-cancel + Unity + airtime rationale      | **Niskie-średnie** (może prosić o rebase na #12180) |
| PR: L2 household pull z caps, favorites ACL, nie global S&F | **Niskie-średnie** (dyskusja protokołu, nie „nie”)  |
| PR: nowa rola w enum                                        | **Średnie-wysokie** (#9942 chce mniej ról)          |
| Czekać z L2 aż 3.0                                          | Zbędne - 3.0 nie blokuje sekretarki                 |

**Największe ryzyko nie merytoryczne, tylko procesowe:** #12180 wejdzie pierwszy → nasz L1 trzeba **zrebase’ować i opisać jako complementary** (secondary / no presence required), nie jako duplikat.

---

## 5. Rekomendowany kierunek (żeby „nie zostać odrzuconym”)

1. **Trzymać locked policy** - secondary default, nie L1-all. To jest nasza tarcza w review.
2. **Przed kolejnym PR upstream:** zmergować `develop`, sprawdzić #12180; w opisie PR napisać porównanie (tabela wyżej).
3. **L2 kontynuować** - live fill-in ≠ offline multi-reader; #10704 to sojusznik narracyjny.
4. **M2 (primary TEXT opt-in)** - pozycjonować jak komentarze „TEXT only / low % traffic”, nie jak ROUTER.
5. **Nie walczyć z #12180** - albo współistnieć, albo zawęzić nasze L1 po merge; obie ścieżki są OK.
6. **Unikać:** nowy role enum, purge-on-ACK, global S&F clone, default all-channel elevate.

---

## 6. Werdykt

| Pytanie                        | Odpowiedź                                                                                                                 |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------------------- |
| Czy idziemy w dobrym kierunku? | **Tak** - ten sam problem co upstream; nasz default jest **bardziej mesh-safe** niż #12180.                               |
| Czy L2 ma sens mimo #12180?    | **Tak** - inna warstwa (historia / offline / cursors).                                                                    |
| Czy odrzucą nas z automatu?    | **Nie**, jeśli nie proponujemy attic=ROUTER i jasno odróżnimy się od (lub zintegrujemy z) #12180.                         |
| Co może „przyciąć” scope?      | Reviewer może powiedzieć: „public już robi #12180, zostawcie secondary/TEXT/L2” - to nadal wygrana, nie odrzucenie wizji. |

Szczegóły linków: `UPSTREAM_WATCH.md`. Policy lock: `ANALYSIS_AND_SIMULATION.md` §7-8.
