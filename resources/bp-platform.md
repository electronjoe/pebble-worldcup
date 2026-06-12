# Pebble Best Practices: Platform Constraints & Hardware

Pebble devices vary widely. Shape/size/color facts below are verified against
the board headers in `PebbleOS/src/fw/board/displays/` (`PBL_DISPLAY_WIDTH/
HEIGHT`, `PBL_ROUND`, `PBL_COLOR`):

| Platform (Models) | Screen | Shape | Colors | Max App (code+heap) | App resources |
| ----------------- | ------ | ----- | ------ | ------------------- | ------------- |
| **Aplite** (Pebble Classic/Steel) | 144×168 | rect | 1‑bit B/W | 24 KB | ~128 KB Flash |
| **Basalt** (Pebble Time/Steel) | 144×168 | rect | 64-color | 64 KB | ~256 KB Flash |
| **Chalk** (Pebble Time Round) | 180×180 | round | 64-color | 64 KB | ~256 KB Flash |
| **Diorite** (Pebble 2) | 144×168 | rect | 1‑bit B/W | 64 KB | ~256 KB Flash |
| **Emery** (Pebble Time 2 / Core Time 2) | 200×228 | rect | 64-color | 128 KB | ~256 KB Flash |
| **Flint** (Core 2 Duo) | 144×168 | rect | 1‑bit B/W | 64 KB | ~256 KB Flash |
| **Gabbro** (in-development, in PebbleOS source) | 260×260 | round | 64-color | — | — |

*Color-capable models support 64 colors (2 bits per RGB channel)【42†L185-L193】. Max-app and resource figures come from the original Pebble docs【31†L76-L83】【48†L64-L72】; the per-platform app-RAM split is generated at firmware build time (`process_management/sdk_memory_limits.template.h`), so treat the KB figures as approximate and verify with the `pebble build` size report.*

## Memory & Resources

Each app is limited to 256 KB of flash resources on Basalt/Chalk and later platforms, and 128 KB on Aplite【48†L64-L72】. Only 256 resource files (images, fonts, etc.) are allowed【48†L64-L72】. Code + heap size is ~24 KB on Aplite vs 64–128 KB on newer boards【31†L76-L83】.

**Best Practice:** Minimize images/fonts size, load only essential resources, and free unused layers/windows to fit these limits. Use `heap_bytes_free()` to monitor memory. The system logs heap usage at exit, which helps detect leaks【45†L122-L131】.

## Cross-Platform Adaptation

Use **compile-time defines** to tailor behavior per hardware:
```c
#if defined(PBL_COLOR)
  text_layer_set_text_color(layer, GColorRed);
#else
  text_layer_set_text_color(layer, GColorWhite);
#endif

window_set_background_color(window, PBL_IF_COLOR_ELSE(GColorJaegerGreen, GColorBlack));
```
This ensures legibility on both color and monochrome screens【1†L89-L95】【1†L99-L102】.

## Round vs Rectangular

Round platforms (Chalk, and Gabbro in newer PebbleOS) require special handling【26†L66-L75】:
- Leave a 2px margin around edges for the bezel【26†L66-L74】.
- Menus on round screens are "center-focused": highlight moves to center【26†L79-L87】.
- Text scrolls by pages, not smooth scroll, to avoid confusing reflow【26†L98-L107】.
- Use `ContentIndicator` (up/down arrows) to show more content【26†L112-L121】.
- Don't blindly port rectangular designs; adapt layouts for round screens【26†L125-L133】.

## Checklist

- Know your target platforms' memory limits (24 KB Aplite vs 64–128 KB newer).
- Use `PBL_IF_COLOR_ELSE`, `PBL_IF_BW_ELSE`, `PBL_PLATFORM_*` macros for cross-platform code.
- Keep flash resources under 128 KB (Aplite) or 256 KB (Basalt/Chalk).
- Stay under the 256 resource file limit.
- Monitor heap with `heap_bytes_free()`.
- Handle round-screen layout differences (margins, pagination, center-focus menus).
