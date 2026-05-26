# Pebble Best Practices: Performance, Memory & Battery

Pebble apps must be lean. Every wakeup, sensor read, and Bluetooth message costs battery.

## Update Sparingly

Don't wake the CPU unnecessarily. For watchfaces, use the slowest tick interval that works (minute updates instead of seconds)【6†L107-L115】:

```c
tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  if(tick_time->tm_sec == 0) { update_time_display(); }
}
```

This keeps the watch asleep ~59x longer, vastly extending battery life【6†L69-L77】. Only use `SECOND_UNIT` if your app *truly* needs per-second updates (e.g. a stopwatch). Allow users to disable any non-essential animation/second hand.

## Animations

Each frame costs wake time. Use `Animation`/`PropertyAnimation` APIs for smooth movements【5†L58-L67】【5†L68-L77】, but animate infrequently: on wrist raise or tap, not every second【6†L69-L77】. For countdowns or clocks, consider ~15–20 FPS. Invalidate layers only when content changes.

## Sensors & Communication

Batch accelerometer/compass readings to reduce wakeups【6†L125-L134】. Set `ACCEL_SAMPLING_10HZ` and request 10 samples per callback so the app wakes ~1x per second【6†L134-L142】. Only listen to sensor taps when needed. Avoid continuous compass or heart-rate monitoring unless actively using it.

## Bluetooth & AppMessage

Frequent AppMessage activity forces the BT chip to stay in high-power mode【8†L174-L183】. Use the default `SNIFF_INTERVAL_NORMAL` and batch data. Cache weather data and only send hourly updates. For large data transfers, switch to `SNIFF_INTERVAL_REDUCED` and back when done【8†L179-L188】. Minimize JSON/message size (compact keys).

## Backlight & Vibration

The backlight LED is power-hungry【8†L203-L211】. Design your UI so as few button presses are needed as possible (use `ActionBarLayer` for quick access rather than deep menus)【8†L195-L204】. If you use `light_enable(true)`, turn it off promptly.

Vibration draws significant current【8†L212-L219】. Use short patterns and let the user disable alerts. Save haptics for truly important notifications. Provide *optional* haptic settings. Use distinct patterns (one short buzz vs. two quick) to convey different events.

## Memory Management

- Always call `XXX_destroy()` for every created UI object.
- Use `heap_bytes_free()` to monitor runtime memory.
- The system logs heap usage at exit — use this to detect leaks【45†L122-L131】.
- Prefer `snprintf` into fixed buffers over dynamic `strcat`.
- Use persistent storage (Storage API) for state to speed up startup.

## Checklist

- Prefer `MINUTE_UNIT` over `SECOND_UNIT` for tick subscriptions.
- Batch sensor readings (e.g. 10 samples per accel callback at 10Hz).
- Batch AppMessage payloads; use `SNIFF_INTERVAL_NORMAL` by default.
- Limit `Light` and `Vibes` API usage; provide user-disable options.
- Animate on events only, not continuously; target ≤20 FPS.
- Profile heap usage via logs and `heap_bytes_free()`.
- Always destroy every created UI object; watch for leaks.
- Use persistent storage to avoid redundant work on startup.
