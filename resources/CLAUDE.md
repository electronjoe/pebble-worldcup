# resources/ — Reference material for Pebble app development

This directory holds reference material vendored alongside the app, **not**
runtime resources (images/fonts) bundled into the watchapp itself — those
live in `package.json`'s `resources` array.

## PebbleOS source (`resources/PebbleOS/`)

Git submodule of [coredevices/PebbleOS](https://github.com/coredevices/PebbleOS).
This is the firmware that runs on the watch; for app development it is the
**ground-truth reference** for every SDK function the app can call. When a
header in `pebble.h` is ambiguous, the implementation under
`PebbleOS/src/fw/applib/` is the authoritative answer.

The PebbleOS repo also has its own `AGENTS.md` and `.claude/CLAUDE.md` —
both are written for *firmware* contributors. They cover board configs, waf
builds, the syscall plumbing for adding new SDK functions, etc. Ignore
those when working on the app; only the contents of `applib/` are relevant
to the app side.

### How an app sees applib

Apps link against `pebble.h` (provided by the Pebble SDK). Each public
function in `pebble.h` corresponds to a wrapper in
`PebbleOS/src/fw/applib/.../<area>.{c,h}` which dispatches through a
syscall into the kernel. When tracing behavior:

1. Find the function in `PebbleOS/src/fw/applib/<area>/<file>.c`.
2. Follow the `sys_*` call into `src/fw/syscall/syscall_<area>.c` for the
   kernel-side implementation.
3. The applib wrapper is what defines argument validation, defaults, and
   user-visible semantics.

### applib subsystem index

Each entry below points to the directory or file under
`PebbleOS/src/fw/applib/`. Most areas pair a `<name>.h` (public API) with
`<name>.c` (wrapper) and a `<name>_private.h` (internal types).

**App lifecycle & process**
- `app.{c,h}` — app entry, event loop (`app_event_loop`), main run loop.
- `app_exit_reason.{c,h}` — set/get reason the app is about to exit.
- `app_launch_reason.{c,h}` — why the app was launched (user, wakeup, etc.).
- `app_launch_button.{c,h}` — which button (if any) launched the app.
- `app_focus_service.{c,h}` — willfocus/didfocus events when modals appear.
- `app_glance.{c,h}` — launcher glance slice (icon + subtitle on launcher).
- `app_heap_util.{c,h}` — heap stats for the app.
- `app_logging.{c,h}` / `logging.c` — `APP_LOG` macro plumbing.
- `app_watch_info.{c,h}` — hardware/firmware version, color, language.
- `platform.h` — `PBL_PLATFORM_*` compile-time platform macros.
- `preferred_content_size.{c,h}` — user's text-size preference.

**Timers, scheduling, sleep**
- `app_timer.{c,h}` — one-shot timers (`app_timer_register`).
- `app_wakeup.{c,h}` — schedule the app to relaunch later (`wakeup_schedule`).
- `tick_timer_service.{c,h}` — minute/second/etc. tick subscription (the
  watchface heartbeat).

**Input & gestures**
- `app_recognizers.{c,h}` — gesture recognizer API surface.
- `ui/recognizer/` — recognizer infrastructure; `tap.{c,h}` is the accel-
  tap recognizer.
- `ui/click.{c,h}` — single/multi/long click handlers attached to windows.
- `touch_service.{c,h}` — touch events (Time 2 / Duo touch-capable boards).

**Sensors**
- `accel_service.{c,h}` — accelerometer subscription, batched samples,
  tap events. `accel_service_private.h` for handler types.
- `compass_service.{c,h}` — compass heading, calibration status.
- `health_service.{c,h}` — steps, sleep, heart rate, activity events.

**Power & system services**
- `app_light.{c,h}` — request the backlight for a short window.
- `backlight_service.{c,h}` — backlight on/off control surface.
- `battery_state_service.{c,h}` — battery percent + charging state.
- `connection_service.{c,h}` — phone/pebblekit connection status changes.
- `cpu_cache.{c,h}` — `memory_cache_flush` for self-modifying buffers (rare).

**UI — windows & layers**
- `ui/window.{c,h}` + `ui/window_stack.{c,h}` — windows, push/pop, the
  modal stack. `app_window_stack.{c,h}` is the app-facing wrapper.
- `ui/layer.{c,h}` — base layer (every visual element is a layer or owns
  one).
- `ui/window_stack_animation*.c` — push/pop transition animations.

**UI — drawable layers**
- `ui/text_layer.{c,h}` — single-block text rendering.
- `ui/bitmap_layer.{c,h}` — static `GBitmap` rendering.
- `ui/rotate_bitmap_layer.{c,h}` / `ui/rotbmp_pair_layer.{c,h}` —
  rotated bitmap rendering (compass dials, hands).
- `ui/path_layer.{c,h}` — `GPath` stroke/fill layer.
- `ui/inverter_layer.{c,h}` — XOR-style inverter overlay.
- `ui/status_bar_layer.{c,h}` — system status bar in app windows.
- `ui/action_bar_layer.{c,h}` — right-edge icon bar bound to up/select/down.
- `ui/progress_layer.{c,h}` / `ui/progress_window.{c,h}` — progress bars.
- `ui/scroll_layer.{c,h}` — vertically scrolling viewport.
- `ui/content_indicator.{c,h}` — chevrons that hint at off-screen content.
- `ui/qr_code.{c,h}` — QR rendering helper.
- `ui/crumbs_layer.{c,h}` — breadcrumb dots.
- `ui/shadows.{c,h}` — soft-shadow helper.

**UI — menus & selection**
- `ui/menu_layer.{c,h}` (+ `menu_cell_layer.h`, `menu_layer_system_cells.c`)
  — scrolling menu with custom cells.
- `ui/simple_menu_layer.{c,h}` — convenience wrapper for a flat menu.
- `ui/option_menu_window.{c,h}` — full-window picker.
- `ui/action_menu_layer.{c,h}` + `action_menu_hierarchy.{c,h}` +
  `action_menu_window.{c,h}` — hierarchical action menu (long-press style).
- `ui/action_toggle.{c,h}` — two-state action UI helper.
- `ui/selection_layer.{c,h}` — multi-digit selection strip.
- `ui/number_window.{c,h}` — pick-a-number window.
- `ui/date_selection_window.{c,h}` / `ui/time_selection_window.{c,h}` /
  `ui/time_range_selection_window.{c,h}` — date/time pickers.

**UI — dialogs (`ui/dialogs/`)**
- `dialog.{c,h}` — base modal dialog.
- `simple_dialog.{c,h}` — icon + text, auto-dismiss.
- `confirmation_dialog.{c,h}` — yes/no.
- `actionable_dialog.{c,h}` — dialog with an action bar.
- `expandable_dialog.{c,h}` — scrollable longer text.
- `bt_conn_dialog.{c,h}` — standard "no phone connection" dialog.

**UI — animation**
- `ui/animation.{c,h}` + `animation_private.h` + `animation_timing.{c,h}` +
  `animation_interpolate.{c,h}` — animation primitives and curves.
- `ui/property_animation.{c,h}` — animate layer properties (frame, origin,
  GColor, etc.).
- `ui/vibes.{c,h}` — vibration patterns (`vibes_short_pulse`, etc.).
- `ui/speaker.{c,h}` — speaker control (boards with a speaker).

**UI — animated images (`ui/kino/`)**
- `kino_player.{c,h}` / `kino_layer.{c,h}` — play a `KinoReel`.
- `kino_reel.{c,h}` + `kino_reel_gbitmap*.{c,h}` + `kino_reel_pdci.{c,h}` /
  `kino_reel_pdcs.{c,h}` / `kino_reel_custom.{c,h}` — reel sources
  (bitmaps, PDC images/sequences, custom).

**Graphics (`graphics/`)**
- `gtypes.{c,h}` — `GColor`, `GPoint`, `GRect`, `GSize`, etc.
- `gcontext.h` + `graphics.{c,h}` — drawing context, fill/stroke/text APIs.
- `graphics_circle.{c,h}` / `graphics_line.{c,h}` / `graphics_bitmap.{c,h}`
  — primitive drawing entry points.
- `gbitmap.c` / `gbitmap_pbi.h` / `gbitmap_png.{c,h}` /
  `gbitmap_sequence.{c,h}` — bitmap data, PNG decode, APNG-like sequences.
- `gpath.{c,h}` + `gpath_builder.{c,h}` — polygon paths.
- `gdraw_command*.{c,h}` — Pebble Draw Command (PDC) vector images and
  sequences; `gdraw_command_transforms.{c,h}` for transforming them.
- `text.h` + `text_layout.c` + `text_render.{c,h}` +
  `text_resources.{c,h}` — text measurement and rendering pipeline.
- `gcolor_definitions.{c,h}` — named `GColor` constants.
- `gtransform.{c,h}` — fixed-point affine transforms for rotation/scale.
- `framebuffer.{c,h}` — direct framebuffer access (advanced).
- `rtl_support.{c,h}` / `arabic_shaping.{c,h}` / `utf8.{c,h}` — RTL and
  multibyte text support.
- `1_bit/` and `8_bit/` — depth-specific blit and primitive backends
  (selected at build time by board color depth).

**Fonts (`fonts/`)**
- `fonts.{c,h}` — `fonts_load_custom_font`, system font lookup.
- `codepoint.{c,h}` — codepoint classification.

**Phone messaging**
- `app_message/app_message.{c,h}` — `AppMessage` dictionaries to/from JS.
  `app_message_inbox.c` and `app_message_outbox.c` are the two directions;
  `app_message_receiver.{c,h}` is the receive-side state machine.
- `app_comm.{c,h}` — legacy app-comm (`app_comm_get_sniff_interval`).
- `app_inbox.{c,h}` / `app_outbox.{c,h}` — lower-level inbox/outbox plumbing
  under app_message.
- `app_sync/` — `AppSync` helper that mirrors a dictionary to a local
  buffer.
- `plugin_service.{c,h}` — companion-app plugin channel.

**Bluetooth (`bluetooth/`)** — direct BLE for apps that need it.
- `ble_app_support.{c,h}` — init/teardown for the BLE app context.
- `ble_central.{c,h}` / `ble_scan.{c,h}` — central-role scan/connect.
- `ble_client.{c,h}` / `ble_service.{c,h}` / `ble_characteristic.{c,h}` /
  `ble_descriptor.h` — GATT client objects.
- `ble_device.{c,h}` — remote device handle.
- `ble_security.{c,h}` — pairing/bonding.
- `ble_ad_parse.{c,h}` — advertisement payload parsing.
- `ble_ibeacon.{c,h}` — iBeacon helper.

**Voice (`voice/`)**
- `dictation_session.{c,h}` — start/stop dictation, receive transcription.
- `voice_window.{c,h}` + `loading_layer.{c,h}` + `transcription_dialog.{c,h}`
  — the built-in dictation UI.

**Smartstrap & worker**
- `app_smartstrap.{c,h}` — smartstrap accessory comms.
- `worker.{c,h}` + `app_worker.{c,h}` — background worker process API
  (`worker_src/c/` in this repo).

**Storage & sync**
- `persist.{c,h}` — key/value persistent storage per app (`persist_read_*`,
  `persist_write_*`).
- `data_logging.{c,h}` — append-only logs flushed to the phone.
- `applib_resource.{c,h}` + `applib_resource_private.h` — runtime resource
  lookup (`resource_get_handle`, `resource_load_byte_range`).

**i18n & C runtime**
- `i18n.{c,h}` — gettext-style string lookup.
- `pbl_std/` — small libc shim: `locale.{c,h}`, `strftime.c`,
  `pbl_std.{c,h}`.
- `template_string.{c,h}` — printf-style template evaluation.
- `preferred_durations.{c,h}` — user's preferred animation durations.

**Misc / advanced**
- `event_service_client.{c,h}` — base for the various `*_service`
  subscribe/unsubscribe modules.
- `unobstructed_area_service.{c,h}` — notification of "Quick View" obscuring
  the screen.
- `pebble_warn_unsupported_functions.h` — compile-time warnings for APIs not
  available on the current platform.
- `legacy2/` — legacy SDK 2.x compatibility shims.
- `rockyjs/` — Rocky.js (JerryScript-based watchface JS); not used by
  C-only apps.
- `moddable/` — Moddable XS JS runtime hooks; same caveat.
- `vendor/` — third-party code used by applib.

## Other useful references inside `PebbleOS/`

### `sdk/` — public SDK generation

- `sdk/defaults/app/` — **the template `pebble new-project` instantiates.**
  Canonical reference for a fresh watchapp: `main.c` (full window/event-
  loop scaffold), `simple.c` (minimal entry point), `worker.c`
  (background-worker template), `index.js` (PebbleKit JS skeleton),
  `package.json` (manifest defaults).
- `sdk/defaults/lib/` — template for libraries created with
  `pebble new-package` (`lib.c`, `lib.h`, `lib.js`).
- `sdk/include/` — PebbleKit JS glue injected into every app's JS bundle
  (`_message_key_wrapper.js`, `_pkjs_message_wrapper.js`,
  `_pkjs_shared_additions.js`). Look here when JS-side `Pebble.*`
  behavior is surprising.
- `sdk/tools/` — `inject_metadata.py` runs during every `pebble build` to
  stamp the binary; `memory_reports.py` produces the app-size report;
  `pebble_package.py` handles `.pbw` packaging.

### `include/` — firmware-side headers

Mostly firmware-internal (BT stack internals in `include/bluetooth/`,
log-hashing in `include/logging/`, chip/firmware metadata in
`include/pebbleos/`) — apps do not link against any of these.

The one app-relevant subtree is **`include/pbl/services/`** — these are
the kernel-side services that applib's `*_service.{c,h}` wrappers
syscall into. Names align 1:1 with applib for the exposed surface
(`accel_manager.h`, `event_service.h`, `tick_timer.h`, `wakeup.h`,
`light.h`, `vibe_pattern.h`, `compositor`, `i18n/`, `persist.h`, etc.),
plus many internal services with no app-side equivalent (`activity/`,
`blob_db/`, `comm_session/`, `notifications/`, `timeline/`, `weather/`,
`alarms/`, `voice_endpoint.h`, …). Useful when tracing what happens
*after* the syscall — but apps cannot call these directly.

### Other

- `docs/reference/` — reference documentation seed material.
- `tools/generate_native_sdk/exported_symbols.json` — exact list of symbols
  exposed to apps, with the SDK revision each was added in. Useful for
  checking whether an API exists on a given firmware version.
- `src/fw/process_management/pebble_process_info.h` — current SDK
  major/minor version.

## Example apps (`resources/examples/`)

Each entry is a standalone Pebble project (git submodule). Use these as
copy-paste-ready references for the listed APIs / patterns.

- **`cards-example/`** — scrolling weather-card UI built on the **Pebble
  Draw Commands (PDC)** API. Best reference for vector graphics
  (`gdraw_command_*`) and the `svg2pdc.py` toolchain in
  `tools/svg2pdc.py` that converts SVGs into the `.pdc` resource format.
- **`ui-patterns/`** — menu launcher into the recommended UI patterns:
  checkbox list, radio-button list, message dialog, choice dialog, list
  hint message, PIN entry. Best reference for `menu_layer`,
  `action_bar_layer`, dialogs, and selection windows.
- **`pebble-faces/`** — download PNGs from the internet via PebbleKit JS,
  decode with bundled uPNG, and display. Reference for `AppMessage` data
  transfer of binary payloads, `netdownload` pattern, and runtime PNG →
  `GBitmap` decoding.
- **`watchface-tutorial/`** — completed result of the official 3-part
  watchface getting-started tutorial; weather watchface with OWM API
  fetch in `src/js/app.js`. Reference for the minimal watchface shell
  (tick timer + text layer + bitmap background) plus the simplest
  end-to-end JS-fetch → C-display flow.
- **`app-font-browser/`** — cycles through every built-in system font.
  Reference for the `fonts_get_system_font` constants and the
  `Clicks` API (`window_single_click_subscribe` for up/down navigation).

## Best practices (`resources/bp-*.md`)

Pebble development best practices, split by lifecycle phase. Load the
relevant file when working on that phase — each is self-contained with
guidance, code examples, and a checklist.

- **[`bp-platform.md`](bp-platform.md)** — Hardware specs (Aplite through
  Emery), memory/resource limits per platform, cross-platform compile-time
  macros (`PBL_IF_COLOR_ELSE`, `PBL_PLATFORM_*`), and round-vs-rectangular
  layout considerations. Consult when choosing target platforms or sizing
  resources.

- **[`bp-sdk-tools.md`](bp-sdk-tools.md)** — SDK setup, `pebble` CLI
  commands (build/install/logs), PebbleKit JS communication
  (`Pebble.sendAppMessage`), Timeline Web API pins, and system fonts.
  Consult when setting up the dev environment or wiring phone↔watch comms.

- **[`bp-ui-design.md`](bp-ui-design.md)** — Standard widgets (MenuLayer,
  ActionBarLayer, ScrollLayer, StatusBarLayer), interaction patterns
  (cards, lists, action bars), typography sizes (28/24/18/14pt), icon
  dimensions, the 64-color palette, `gcolor_legible_over()`, and good/bad
  design examples. Consult when designing screens or choosing UI
  components.

- **[`bp-performance.md`](bp-performance.md)** — Tick timer intervals
  (MINUTE_UNIT vs SECOND_UNIT), animation budgets, sensor batching,
  AppMessage/BT power management (SNIFF_INTERVAL), backlight/vibration
  power costs, and memory management (`heap_bytes_free`, destroy
  patterns). Consult when optimizing battery life or debugging memory
  issues.

- **[`bp-testing.md`](bp-testing.md)** — Emulator vs device testing, `APP_LOG`
  / `console.log` usage, GDB debugging, heap profiling, accessibility
  (legibility, haptics, colorblind support, dictation fallback), and
  in-context testing (one-handed, varied lighting). Consult when writing
  tests or validating on hardware.

- **[`bp-release.md`](bp-release.md)** — PBW packaging (image compression,
  debug symbol removal), resource size limits, `package.json` versioning,
  Timeline API key setup (sandbox vs production), and store submission.
  Consult when preparing a release.

The original combined document is [`BEST_PRACTICES.md`](BEST_PRACTICES.md).

## When to look at PebbleOS vs. the public SDK headers

- Use `pebble.h` (from the Pebble SDK) as the API surface you are coding
  against — it is what the toolchain links.
- Drop into `resources/PebbleOS/src/fw/applib/` when you need to know
  *how* a function behaves (argument bounds, what events fire, ordering,
  error returns), since `pebble.h` rarely documents these in depth.
