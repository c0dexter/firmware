# Use cases - CLIENT_BASE household base

Aligned with locked policy in `ANALYSIS_AND_SIMULATION.md` / `README.md`.

## UC1 - Private secondary live (default L1) - primary pain for this project

**Given** roof `CLIENT_BASE`, home favorited, secondary `#CHAT` configured on roof and home  
**When** mesh (or peer) sends broadcast on `#CHAT` and roof receives it  
**Then** roof does **not** cancel rebroadcast like CLIENT; late TX so home can hear the roof  
**And** phone on roof still sees RX as today

_Sim (private-only urban): ~59% → ~86% home delivery._

## UC2 - Primary / LongFast without opt-in

**Given** default config (primary TEXT elevate **off**)  
**When** broadcast on primary  
**Then** behavior stays CLIENT-like for cancel (best-effort); may still be “w kratkę” for indoor home  
**And** docs state this honestly (no false ROUTER promise)

## UC3 - Primary TEXT with opt-in

**Given** operator enables primary TEXT elevate  
**When** TEXT on primary  
**Then** same late never-cancel treatment as secondary for those packets  
**And** operator accepts higher public airtime (documented)

## UC4 - Secondary missing on base

**Given** `#CHAT` on home only, not on roof  
**When** `#CHAT` traffic is on air  
**Then** base cannot decrypt, L1-elevate, or L2-store it

## UC5 - Two household members, staggered pull (L2)

**Given** shared buffer, TTL 24h, cursors; both favorited on roof  
**When** Anna sends `CLIENT_HISTORY` then Bart later  
**Then** each gets TEXT unicasts from their own cursor; ring not purged; second poller during dump gets `ROUTER_BUSY`

_Sim: purge-on-ACK would drop ~24 shared msgs for Bart. Wire: `schemas.md` L2 poll._

## UC6 - Home offline then returns (L2)

**Given** home radio off/out of range for hours while secondary traffic hits roof  
**When** home returns and sends `CLIENT_HISTORY` (favorite)  
**Then** L2 delivers buffered TEXT (`hop_limit=0`) up to `history_return_max` per session; L1 alone recovers nothing for the offline window

_Sim: 6h × 4 msg/h → 24 unicast recovery cost, local only._

## UC7 - TTL expired

**Given** message older than TTL / overwritten  
**When** favorite polls  
**Then** message absent; L3 peer catch-up is future work

## UC8 - Must not become city ROUTER by default

**Given** default CLIENT_BASE  
**When** serving household  
**Then** public primary is not fully elevated; secondary scoped only  
**And** L1-all / ROUTER-equivalent is not the default path
