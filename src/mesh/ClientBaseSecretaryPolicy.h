#pragma once

#include "memory/MemClass.h"
#include "mesh/generated/meshtastic/config.pb.h"
#include <cstdint>

/**
 * CLIENT_BASE L2 secretary - D2 caps + enable gates (L2_PLAN).
 * Pure helpers for Unity; module wiring stays in ClientBaseSecretaryModule.
 */

/** 24 h TTL (locked sim / L2_PLAN D2). */
static constexpr uint32_t CLIENT_BASE_SECRETARY_TTL_SEC = 24UL * 3600UL;

/** Cursor slots / served household ACL size. */
static constexpr uint8_t CLIENT_BASE_SECRETARY_MAX_FAVORITES = 8;

/** Hard cap on unicast frames per poll session (airtime). */
static constexpr uint16_t CLIENT_BASE_SECRETARY_HISTORY_RETURN_MAX = 32;

/** Ring sizes by MEM_CLASS - dump time bounded even on LARGE. */
static constexpr uint16_t CLIENT_BASE_SECRETARY_RING_SMALL = 64;
static constexpr uint16_t CLIENT_BASE_SECRETARY_RING_MEDIUM = 96;
static constexpr uint16_t CLIENT_BASE_SECRETARY_RING_LARGE = 256;

/** STM32WL / TINY: no L2 secretary. */
inline bool clientBaseSecretaryPlatformSupported()
{
#if MESHTASTIC_MEM_CLASS <= MEM_CLASS_TINY
    return false;
#else
    return true;
#endif
}

/** Role + platform gate (favorite ACL applied later on poll). */
inline bool clientBaseSecretaryShouldEnable(meshtastic_Config_DeviceConfig_Role role)
{
    return role == meshtastic_Config_DeviceConfig_Role_CLIENT_BASE && clientBaseSecretaryPlatformSupported();
}

/** Default ring capacity for this build's MEM_CLASS (0 = disabled). */
inline uint16_t clientBaseSecretaryRingCapacity()
{
#if MESHTASTIC_MEM_CLASS <= MEM_CLASS_TINY
    return 0;
#elif MESHTASTIC_MEM_CLASS == MEM_CLASS_SMALL
    return CLIENT_BASE_SECRETARY_RING_SMALL;
#elif MESHTASTIC_MEM_CLASS == MEM_CLASS_MEDIUM
    return CLIENT_BASE_SECRETARY_RING_MEDIUM;
#else
    return CLIENT_BASE_SECRETARY_RING_LARGE;
#endif
}
