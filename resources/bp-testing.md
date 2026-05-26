# Pebble Best Practices: Testing, Debugging & Accessibility

## Emulator vs Device

Always test on both the official emulator (runs C code) and actual hardware (via USB/Dev Connection). Different models behave differently (especially round screens). Use `pebble install --emulator <platform>` and `pebble install --phone` for each target.

## Logging

Use `APP_LOG()` (C) and `console.log()` (JS) during development to track behavior【45†L65-L75】【45†L90-L98】:

```c
APP_LOG(APP_LOG_LEVEL_DEBUG, "Sensor data: x=%d, y=%d, z=%d", data.x, data.y, data.z);
```

Logs appear via `pebble logs`. Heavy logging slows AppMessage and burns battery【45†L139-L147】 — remove or disable verbose logs in production.

## Breakpoints & Profiling

The emulator allows GDB debugging【44†L69-L77】. Use it to step through crashes. Build artifacts include a "Size" report (heap/stack usage) after each run【45†L122-L131】. Monitor heap usage to catch leaks (always call `window_destroy()` after `window_create()`).

## Automation & Sideloading

Use CI or your development workflow to compile/test across all platforms. CloudPebble (web IDE) or the command line can incrementally update an app on your phone/watch for rapid iteration.

## Accessibility

**Legibility:** Ensure text is large and high-contrast. For colorblind users, don't rely solely on color cues (e.g. red-green); accompany with symbols or text. Use `gcolor_legible_over()` for text on colored backgrounds【27†L152-L156】. Pebble supports UTF-8; use system fonts for Western scripts, or custom fonts for CJK (but beware memory).

**Haptics:** Use vibration for feedback — a single short pulse for success, longer buzz or repeats for alerts. Keep it meaningful but subtle. Respect quiet modes.

**User Input:** Voice dictation is available on microphone models via `dictation_session_start()`【24†L119-L128】. Always confirm before accepting dictation, and fall back to multiple-choice or number wheels if dictation fails. For touch models, support large touch targets and haptic feedback on button presses.

**Testing in Context:** People use watches on the move. Test while walking or performing tasks. Ensure it works with one hand (button access) and in different lighting (glare vs dark).

## Checklist

- Test every build on all target hardware (aplite/basalt/chalk) and emulator.
- Use `APP_LOG()` / `console.log()` liberally during dev; strip for release.
- Use `pebble logs` to monitor crashes and message failures.
- Monitor heap usage from logs and size reports.
- Use GDB on the emulator for crash investigation.
- Provide haptic alternatives to visual notifications.
- Avoid small text; ensure contrast on all screen types.
- Test with one hand, in motion, and under varied lighting.
- Test voice dictation fallback paths.
