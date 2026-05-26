# Pebble Best Practices: Platform Constraints & Hardware

Pebble devices vary widely. A summary of key specs is in the Pebble docs【31†L76-L83】:

| Platform (Models)              | Screen      | Colors    | Max App (code+heap) | Available RAM     |
| ------------------------------ | ----------- | --------- | ------------------- | ----------------- |
| **Aplite** (Pebble Classic/Steel) | 144×168 B×W | 1‑bit B/W | 24 KB               | ~96 KB Flash      |
| **Basalt** (Pebble Time/Steel)    | 144×168     | 64-color  | 64 KB               | ~256 KB Flash     |
| **Chalk** (Pebble Time Round)     | 180×180     | 1‑bit B/W | 128 KB              | ~256 KB Flash     |
| **Diorite/Flint** (Pebble 2/2 Duo) | 200×228     | 64-color  | 128 KB              | (Star-MC1 240MHz) |
| *(Emery – Time 2)*               | *round?*    | *64-color*| *128 KB*            | *~256 KB Flash*   |

*Color-capable models support 64 colors (2 bits per RGB channel)【42†L185-L193】. (Data from Pebble Hardware Info【31†L76-L83】.)*

## Memory & Resources

Each app is limited to 256 KB of flash resources on Basalt/Chalk (144×168 color or 180×180 mono) and 128 KB on Aplite (B/W)【48†L64-L72】. Only 256 resource files (images, fonts, etc.) are allowed【48†L64-L72】. Code + heap size is ~24 KB on Aplite vs 64–128 KB on newer boards【31†L76-L83】.

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

The Time Round (Chalk) requires special handling【26†L66-L75】:
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
