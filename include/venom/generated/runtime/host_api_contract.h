#pragma once
/* Generated from contracts/host-api.json. Do not edit. */
#define VENOM_HOST_BRIDGE_ABI 3u
typedef enum venom_host_call_id {
  VENOM_HOST_CALL_DOM_QUERY = 1u,
  VENOM_HOST_CALL_DOM_MUTATE = 2u,
  VENOM_HOST_CALL_EVENTS_LISTEN = 3u,
  VENOM_HOST_CALL_TIMER_SCHEDULE = 4u,
  VENOM_HOST_CALL_STORAGE_ACCESS = 5u,
  VENOM_HOST_CALL_FETCH_BLOCKED = 6u,
  VENOM_HOST_CALL_CONSOLE_WRITE = 7u,
} venom_host_call_id;

