# Schemas - CLIENT_BASE

Aligned with locked policy (`ANALYSIS_AND_SIMULATION.md`).

## Today (broken expectation)

```mermaid
sequenceDiagram
  participant Mesh
  participant Roof as CLIENT_BASE roof
  participant Phone as Phone on roof
  participant Home as Home client
  Mesh->>Roof: broadcast (from not favorite)
  Roof->>Phone: local RX to app
  Note over Roof: queues rebroadcast as CLIENT
  Mesh->>Roof: hears distant dupe
  Roof--xHome: cancel TX (home never hears)
```

## Target L1 default (secondary only)

```mermaid
sequenceDiagram
  participant Mesh
  participant Roof as CLIENT_BASE roof
  participant Home as Favorited home
  Mesh->>Roof: secondary broadcast (channel on base)
  Note over Roof: late never-cancel (not CLIENT cancel race)
  Roof->>Home: late rebroadcast
  Note over Roof: primary stays CLIENT-like unless opt-in
```

## Policy decision (sim-backed)

```mermaid
flowchart TD
  A[Packet RX on CLIENT_BASE] --> B{Secondary configured on base?}
  B -->|yes| C[L1: late never-cancel]
  B -->|no| D{Primary TEXT and opt-in on?}
  D -->|yes| C
  D -->|no| E[CLIENT cancel-on-dupe]
  C --> F{Secondary TEXT for L2?}
  F -->|yes| G[Enqueue shared ring]
  F -->|no| H[No store]
  G --> I[Favorite poll: unicast from per-node cursor]
```

## Target L2 (secretary, multi-reader)

```mermaid
sequenceDiagram
  participant Mesh
  participant Roof as Base buffer
  participant A as Favorite A
  participant B as Favorite B
  Mesh->>Roof: secondary TEXT
  Roof->>Roof: enqueue shared ring
  A->>Roof: poll
  Roof->>A: unicast; advance cursor_A
  Note over Roof: payload remains
  B->>Roof: poll later
  Roof->>B: unicast from cursor_B
```

## L2 poll wire (D1=A - StoreForward RR)

Control on `STORE_FORWARD_APP`. Payloads as normal `TEXT_MESSAGE_APP` unicast (`hop_limit=0`) on the **stored secondary channel**. Classic S&F server must stay off on the same node.

```mermaid
sequenceDiagram
  participant Home as Favorite CLIENT
  participant Roof as CLIENT_BASE secretary
  Home->>Roof: CLIENT_HISTORY (S&F)
  alt not favorite
    Note over Roof: silent ignore
  else busy (other dump)
    Roof->>Home: ROUTER_BUSY
  else empty cursor
    Roof->>Home: ROUTER_HISTORY (n=0)
  else started
    Roof->>Home: ROUTER_HISTORY (n≤history_return_max)
    loop each message spaced ~2.5s
      Roof->>Home: STORE_FORWARD ROUTER_TEXT_* unicast hop_limit=0 (same secondary; forged from)
      Note over Roof: advance cursor_Home only
    end
  end
  Home-->>Roof: CLIENT_ABORT (optional mid-dump)
```

## Multi-reader cursors

```
ring: [m0 m1 m2 m3 m4 ...]  (TTL / capacity drops oldest)
cursor[Anna] = 2
cursor[Bart] = 0
```

## Airtime intuition (from sim)

```
today .............. ~54 TX / 100 heard
L1 secondary-only .. ~60 TX / 100 heard   ← default target
L1-all / ROUTER* ... ~92 TX / 100 heard   ← avoid as default
```
