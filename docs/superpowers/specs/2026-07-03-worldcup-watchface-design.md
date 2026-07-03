# World Cup 2026 Watch Face — Design

**Date:** 2026-07-03
**Target:** Pebble 2 Duo (flint, 144×168, 1-bit black & white), watchface

## Goal

A watch face showing the time plus the current or next FIFA World Cup 2026
match, updated periodically from the internet. Match data comes from
[openfootball/worldcup.json](https://github.com/openfootball/worldcup.json)
(`2026/worldcup.json`). Knockout pairings are unknown until earlier results
land, so the schedule must be refetched periodically and cached.

Visual reference: `screenshot.png` (AI render) — status bar, large time,
"WORLD CUP" title, pixel-art centerpiece, bordered match box at the bottom.

## Decisions made during brainstorming

- **In-progress matches:** show `USA VS BRA` + `NOW` — no live score
  (openfootball updates lag real time; scores would mislead).
- **Centerpiece art:** simplified — trophy + soccer ball only, not the full
  stadium scene from the render.
- **Kickoff times:** converted to the watch's local time zone.
- **Platforms:** flint only for now.
- **Architecture:** phone sends *all known future matches* to the watch;
  the watch flips between them locally (Approach 2 below).

## Architecture

```
openfootball JSON (GitHub raw)
        │  HTTPS fetch, every 6h + on demand
        ▼
PebbleKit JS (phone)          — parse, filter future matches, encode
        │  AppMessage: packed byte array (~10 B/match)
        ▼
Watch C app                   — persist queue, pick display match per
        │                       minute tick, no phone needed to flip
        ▼
Screen                        — time + match box with bundled flag bitmaps
```

Alternatives considered:
1. *Phone picks the single display match* — rejected: watch goes stale at
   exactly the moments (kickoff, full-time) the user glances at it.
2. *Runtime flag download (pebble-faces pattern)* — rejected: the 48
   qualified teams are fixed; flags can be bundled at build time.

## Screen layout (top → bottom, 144×168)

| Region | Height | Content |
|---|---|---|
| Status bar | ~16px | `SAT JUN 14` left; battery % + icon right; BT-disconnected icon when phone link is down |
| Time | ~50px | `12:56` in `FONT_KEY_LECO_42_NUMBERS` (v1); dot-matrix custom font is a later polish step |
| Title | ~18px | `WORLD CUP` flanked by small soccer-ball bitmaps |
| Art | ~40px | 1-bit pixel art: World Cup trophy + soccer ball, centered |
| Match box | ~40px | Bordered rect. Line 1: `[flag] USA VS BRA [flag]`. Line 2: `TODAY 3:00 PM` / `TOMORROW 12:00 PM` / `SUN 12:00 PM` / `NOW` |

Match-box states:
- **Upcoming match:** team codes + flags, kickoff in watch-local time.
  Date prefix: `TODAY`, `TOMORROW`, else 3-letter weekday (kickoffs are
  always within a week during the tournament).
- **In progress** (kickoff ≤ now < kickoff + 2.5h): same teams, `NOW`.
- **Unknown team** (bracket placeholder): placeholder code if ≤3 chars
  (e.g. `W83`), else `TBD`; generic `?` flag.
- **No future matches** (tournament over / data exhausted):
  `NO UPCOMING GAMES`.

## Phone side (PebbleKit JS, `src/pkjs/index.js`)

- **Fetch:** `worldcup.json` from GitHub raw via XHR on `ready`, then every
  6 hours, and whenever the watch requests a refresh. Last-good response
  cached in `localStorage` (survives JS restarts, serves as fallback on
  fetch failure).
- **Parse:** each match's `date` (`"2026-07-03"`) + `time` (`"18:00 UTC-4"`)
  → UTC epoch seconds. Matches missing a usable time default to 12:00 UTC
  (defensive; all current entries have times).
- **Filter:** keep matches with `kickoff + 3h > now`, sorted chronologically
  — this includes an in-progress match. Send **all** of them. (The 3h JS
  window is deliberately wider than the watch's 2.5h display window, so the
  watch always has strictly more data than it will show.)
- **Team codes:** bundled static name→FIFA-code map generated from
  `worldcup.teams.json` (fetched once at build time, not at runtime).
  Unmapped `team1`/`team2` strings are bracket placeholders: use the raw
  string if ≤3 chars, else `TBD`.
- **Encode:** single AppMessage carrying one byte-array value:
  `count (1 B)`, then per match `kickoff epoch (4 B little-endian)` +
  `code1 (3 B ASCII)` + `code2 (3 B ASCII)` = 10 B/match. 104 matches ≈
  1041 B. Watch inbox sized 4096 B.
- **Message keys:** `MATCHES` (byte array, phone→watch), `REQUEST_REFRESH`
  (watch→phone).

## Watch side (C, `src/c/`)

- `package.json`: `watchapp.watchface: true`, `displayName: "World Cup"`,
  `targetPlatforms: ["flint"]`, message keys above.
- **State:** in-RAM array of `{time_t kickoff; char code1[4]; char code2[4];}`
  parsed from the byte array.
- **Persistence:** raw payload chunked across 256-byte persist keys
  (`PERSIST_DATA_MAX_LENGTH`), plus a length key. Loaded on launch so the
  face is populated before the phone connects. New payload overwrites.
- **Tick handler (MINUTE_UNIT):** redraw clock; select display match =
  first entry with `now < kickoff + 2.5h`. Render `NOW` if
  `now ≥ kickoff`, else the local-time kickoff (C `localtime()` — watch TZ
  is synced by the system). If no entry qualifies and the queue is
  non-empty-but-exhausted, send `REQUEST_REFRESH` (at most once per hour).
- **Flags:** `code → RESOURCE_ID_FLAG_<code>` lookup table (generated
  alongside the flag images); missing code → generic `?` flag.
- **Services:** battery state (status bar %), connection service (BT icon),
  tick timer. No second ticks, no animations — battery-friendly.

## Flag pipeline (build-time, `tools/gen_flags.py`)

- Python + Pillow script, run manually, outputs committed to the repo.
- For each of the 48 teams (from a checked-in copy of
  `worldcup.teams.json`): download the flag (flagcdn.com by ISO code
  derived from the FIFA code mapping), resize to 20×12, Floyd–Steinberg
  dither to 1-bit, save `resources/images/flag_<FIFA>.png`.
- Also emits: the `resources.media` entries for `package.json` and a
  generated C table `flag_resources.h` mapping FIFA code strings to
  resource IDs.
- Plus one hand-made `flag_TBD.png` (`?` glyph) and the soccer-ball and
  trophy art (hand-drawn 1-bit PNGs, not script-generated).

## Error handling

| Failure | Behavior |
|---|---|
| Fetch fails / no network | JS serves `localStorage` cache; retries next 6h cycle |
| Phone disconnected | Watch runs off persisted queue; BT icon shown; only new bracket resolutions are missed |
| Payload malformed / empty | Watch keeps previous persisted queue |
| Queue exhausted | `NO UPCOMING GAMES`; hourly `REQUEST_REFRESH` while connected |
| Unknown team string | Placeholder code + `?` flag |

## Testing

- **JS parsing/encoding:** standalone Node test (`tools/test_parser.js` or
  similar) running the parse/filter/encode functions against a fixture
  copy of `worldcup.json` — covers timezone math (`UTC-6` etc.), the
  future-match filter boundary, and placeholder handling. The pkjs code
  keeps these functions pure/exportable to allow this.
- **Emulator:** `pebble build && pebble install --emulator flint`;
  `pebble logs` for both C `APP_LOG` and JS `console.log`. Verify: cold
  start (no persist), populated queue, match flip at kickoff+2.5h (emulator
  time manipulation or short test override), disconnected rendering.
- **Hardware:** `pebble install --cloudpebble` on the Pebble 2 Duo.

## Out of scope (possible follow-ups)

- Dot-matrix custom time font matching the render.
- Live scores (data source lags; revisit if a better source appears).
- Other platforms (emery/gabbro/legacy) and round-screen layout.
- Full stadium-scene artwork.
