# Flick-to-Browse Upcoming Games — Design

**Date:** 2026-07-03
**Status:** Approved

## Goal

Let the wearer step through upcoming World Cup games from the watchface. The
original request was Up/Down button stepping, but watchfaces never receive
button events (the OS reserves Up/Down for timeline navigation and ignores
`window_set_click_config_provider`). Decision: stay a watchface and use the
accelerometer tap service — a wrist flick steps forward through the queue,
wrapping past the last game back to the soonest, and the display snaps back to
the soonest game 10 seconds after the last flick.

## Decisions

- **Input:** any `AccelTapService` event (any axis/direction) steps forward
  one game. Flicks cannot reliably distinguish direction, so there is no
  backward step; wrap-around plus the short timeout covers recovery.
- **Reset:** 10 seconds after the last flick, the display returns to the
  soonest game. Wrapping past the end also lands on the soonest game.
- **Indicator:** while browsing (offset > 0) the match box shows a small
  `+N` marker so a glance mid-browse is not mistaken for the actual next
  game. No marker at offset 0.

## Changes

### `src/c/match_store.h` / `match_store.c`

- New `const Match *match_store_display_at(time_t now, int offset)` —
  generalizes the existing first-displayable scan: skip matches whose display
  window (`kickoff + MATCH_DISPLAY_WINDOW_SEC`) has passed, then return the
  `offset`-th remaining match, or `NULL` if `offset` runs past the end.
- `match_store_display(now)` becomes a wrapper for `offset = 0` (existing
  call sites unchanged).
- New `int match_store_upcoming_count(time_t now)` — number of displayable
  matches; used for wrap-around and no-op detection.
- `match_format_when`: new branch for kickoffs 7+ days out — print an
  uppercased month/day (e.g. `JUL 14`) instead of a weekday name, which is
  only unambiguous within a week.

### `src/c/worldcup.c`

New state: `static int s_browse_offset;` and `static AppTimer *s_browse_timer;`

- **Flick handler:** subscribe `accel_tap_service_subscribe` in `prv_init`,
  unsubscribe in `prv_deinit`. On tap:
  `s_browse_offset = (s_browse_offset + 1) % match_store_upcoming_count(now)`,
  refresh the display, and (re)schedule the reset timer. If 0 or 1 games are
  displayable, flicks are no-ops.
- **Reset timer:** one `app_timer_register(10000, ...)`, rescheduled on each
  flick. On fire: NULL the handle, reset offset to 0, refresh, redraw.
- **Display:** `prv_refresh_display` passes `s_browse_offset` to
  `match_store_display_at`. If that returns `NULL` while offset > 0 (queue
  shrank mid-browse), reset the offset to 0 and retry.
- **Marker:** when offset > 0, `prv_matchbox_update` draws `+N` in
  GOTHIC_14, small and clear of the flags and text (exact coordinates chosen
  during implementation; candidate spot is the bottom-left of the box beside
  the when-text line).
- **Minute tick:** unchanged — it already calls `prv_refresh_display`, which
  re-renders whatever offset is active.

## Error handling

- Empty store / exhausted queue: existing `NO UPCOMING GAMES` path is
  unchanged; flicks are no-ops.
- New match payload arriving mid-browse: `prv_inbox_received` resets
  `s_browse_offset` to 0 — the list may have reordered, and a stale offset
  would silently point at a different game.

## Testing

- Emulator (`flint`): `pebble emu-tap` to simulate flicks — step forward,
  marker appears, wrap-around returns to soonest, 10 s snap-back fires.
- `TEST_TIME_OFFSET` (existing hook in `worldcup.c`) to exercise the
  7+ day date-format branch and the queue-exhausted no-op.
- Physical sanity check on the emery watch (pbw already targets emery).
