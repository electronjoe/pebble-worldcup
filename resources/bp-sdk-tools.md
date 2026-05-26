# Pebble Best Practices: SDK, APIs & Toolchain

## Pebble SDK & RTOS

Apps are written in C (SDK 3/4) or JavaScript (PebbleKit JS). The RTOS is proprietary but supports a Window/Layer UI system. Leverage system UI elements and services (Accelerometer, AppMessage, etc.).

The common system font ("Raster Gothic Condensed") is optimized for Pebble's displays【34†L133-L136】. Custom fonts may be used but consume RAM. The SDK provides `fonts_get_system_font` and `fonts_load_custom_font` APIs【34†L129-L138】.

## PebbleKit JS & Cloud APIs

A Pebble app can include a phone-side JS component. Use **Pebble.sendAppMessage()** in JS to communicate with the watch【45†L98-L105】. JS code can fetch web data (HTTP) using AJAX. The watch's REST-like **Timeline Web API** lets your server push "pins" (events/notifications) to users' watches【22†L54-L63】.

```js
console.log('Sending data to Pebble...');
Pebble.sendAppMessage({'KEY': value},
  function(e) { console.log('Send successful!'); },
  function(e) { console.log('Send FAILED!'); }
);
```
【45†L98-L105】

**Pins** are JSON objects with title, time, icons, colors, actions, etc.【23†L72-L81】【23†L130-L139】. Manage communications carefully: large or frequent AppMessages can drain power and must be batched or throttled. Always check `AppMessage` success/failure callbacks and cache data locally (Storage API) to avoid unnecessary round-trips【6†L125-L134】【8†L179-L188】.

## CLI Workflow

Use the `pebble` command-line tool for building, installing, and debugging:

| Command | Purpose |
|---------|---------|
| `pebble build` | Compile the app |
| `pebble install --emulator basalt` | Run on simulator |
| `pebble install --phone` | Install on device |
| `pebble install --logs` | Install and stream logs |
| `pebble logs` | Stream C and JS logs |

Enable Developer Connection on your phone to use `pebble logs`. In C, use `APP_LOG()` to print debug info; in JS use `console.log()`. All logs (C and JS) appear via `pebble logs`【45†L112-L120】.

Note: logging over Bluetooth is costly in power【45†L139-L147】, so disable verbose logs before releasing. You can also use GDB on the emulator for stepping through code【44†L69-L77】. The SDK includes a Timeline simulator that refreshes every 30s (no internet needed)【21†L0-L7】.

## Checklist

- Use `pebble build` / `pebble install` for the build-deploy-debug cycle.
- Use `APP_LOG()` (C) and `console.log()` (JS) during development, remove for release.
- Batch AppMessage payloads; always handle success/failure callbacks.
- Cache data locally (Storage API) to reduce round-trips.
- Use compile-time defines (`PBL_COLOR`, `PBL_ROUND`, etc.) to tailor per-hardware behavior.
