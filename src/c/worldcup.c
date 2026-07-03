#include <pebble.h>
#include <ctype.h>
#include "match_store.h"
#include "flag_resources.h"

/* Seconds added to time(NULL); set temporarily (e.g. 30 * 86400) to test
 * queue-exhausted rendering and match flips, then set back to 0. */
#define TEST_TIME_OFFSET 0

static time_t prv_now(void) {
  return time(NULL) + TEST_TIME_OFFSET;
}

static Window *s_window;
static TextLayer *s_time_layer;
static char s_time_buf[8];

static Layer *s_matchbox_layer;
static GBitmap *s_flag1, *s_flag2;
static char s_flag1_code[4], s_flag2_code[4];
static char s_teams_buf[16];
static char s_when_buf[24];
static const Match *s_display_match;
static time_t s_last_refresh_req = 0;
static int s_browse_offset;
static AppTimer *s_browse_timer;

static Layer *s_status_layer;
static Layer *s_title_layer;
static Layer *s_art_layer;
static GBitmap *s_ball_small, *s_ball_large, *s_trophy, *s_bt_icon;
static char s_date_buf[16];

static void prv_refresh_display(void);
static void prv_matchbox_update(Layer *layer, GContext *ctx);

static void prv_update_time(void) {
  time_t now = time(NULL);
  struct tm *lt = localtime(&now);
  strftime(s_time_buf, sizeof(s_time_buf),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", lt);
  if (!clock_is_24h_style() && s_time_buf[0] == '0') {
    memmove(s_time_buf, s_time_buf + 1, strlen(s_time_buf));
  }
  text_layer_set_text(s_time_layer, s_time_buf);
}

static void prv_update_date(void) {
  time_t now = prv_now();
  struct tm *lt = localtime(&now);
  strftime(s_date_buf, sizeof(s_date_buf), "%a %b %e", lt);
  for (char *p = s_date_buf; *p; p++) *p = toupper((unsigned char)*p);
}

static void prv_status_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);

  graphics_draw_text(ctx, s_date_buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(4, -2, 76, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  BatteryChargeState charge = battery_state_service_peek();
  char pct[6];
  snprintf(pct, sizeof(pct), "%d%%", charge.charge_percent);
  graphics_draw_text(ctx, pct, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(b.size.w - 54, -2, 32, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  GRect batt = GRect(b.size.w - 18, 4, 14, 8);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, batt);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(batt.origin.x + batt.size.w, batt.origin.y + 2, 2, 4),
                     0, GCornerNone);
  int fill = (batt.size.w - 4) * charge.charge_percent / 100;
  graphics_fill_rect(ctx, GRect(batt.origin.x + 2, batt.origin.y + 2, fill, 4),
                     0, GCornerNone);

  if (!connection_service_peek_pebble_app_connection() && s_bt_icon) {
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    graphics_draw_bitmap_in_rect(ctx, s_bt_icon, GRect(82, 2, 7, 11));
  }
}

static void prv_title_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, "WORLD CUP",
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(0, -4, b.size.w, 20),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  if (s_ball_small) {
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    graphics_draw_bitmap_in_rect(ctx, s_ball_small, GRect(18, 5, 9, 9));
    graphics_draw_bitmap_in_rect(ctx, s_ball_small, GRect(b.size.w - 27, 5, 9, 9));
  }
}

static void prv_art_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  int group_w = 24 + 6 + 16;  /* trophy + gap + ball */
  int x0 = (b.size.w - group_w) / 2;
  if (s_trophy) graphics_draw_bitmap_in_rect(ctx, s_trophy, GRect(x0, 1, 24, 35));
  if (s_ball_large) {
    graphics_draw_bitmap_in_rect(ctx, s_ball_large,
                                 GRect(x0 + 24 + 6, b.size.h - 18, 16, 16));
  }
}

static void prv_battery_handler(BatteryChargeState charge) {
  if (s_status_layer) layer_mark_dirty(s_status_layer);
}

static void prv_connection_handler(bool connected) {
  if (s_status_layer) layer_mark_dirty(s_status_layer);
}

static void prv_maybe_request_refresh(time_t now) {
  if (!connection_service_peek_pebble_app_connection()) return;
  if (s_last_refresh_req != 0 && now - s_last_refresh_req < 3600) return;
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) return;
  dict_write_uint8(iter, MESSAGE_KEY_REQUEST_REFRESH, 1);
  app_message_outbox_send();
  s_last_refresh_req = now;
  APP_LOG(APP_LOG_LEVEL_INFO, "requested refresh from phone");
}

static GBitmap *prv_load_flag(const char *code, GBitmap *old, char *cached_code) {
  if (old && strncmp(code, cached_code, 3) == 0) return old;
  if (old) gbitmap_destroy(old);
  strncpy(cached_code, code, 3);
  cached_code[3] = '\0';
  return gbitmap_create_with_resource(flag_resource_for_code(code));
}

static void prv_browse_reset(void *context) {
  s_browse_timer = NULL;
  s_browse_offset = 0;
  prv_refresh_display();
}

static void prv_tap_handler(AccelAxisType axis, int32_t direction) {
  int count = match_store_upcoming_count(prv_now());
  if (count <= 1) return;  /* nothing to browse */
  s_browse_offset = (s_browse_offset + 1) % count;
  prv_refresh_display();
  if (s_browse_timer) {
    app_timer_reschedule(s_browse_timer, 10000);
  } else {
    s_browse_timer = app_timer_register(10000, prv_browse_reset, NULL);
  }
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  prv_update_time();
  prv_update_date();
  if (s_status_layer) layer_mark_dirty(s_status_layer);
  prv_refresh_display();
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_time_layer = text_layer_create(GRect(0, 14, bounds.size.w, 50));
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_ball_small = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BALL_SMALL);
  s_ball_large = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BALL_LARGE);
  s_trophy = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TROPHY);
  s_bt_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_OFF);

  s_status_layer = layer_create(GRect(0, 0, bounds.size.w, 16));
  layer_set_update_proc(s_status_layer, prv_status_update);
  layer_add_child(window_layer, s_status_layer);

  s_title_layer = layer_create(GRect(0, 66, bounds.size.w, 18));
  layer_set_update_proc(s_title_layer, prv_title_update);
  layer_add_child(window_layer, s_title_layer);

  s_art_layer = layer_create(GRect(0, 86, bounds.size.w, 38));
  layer_set_update_proc(s_art_layer, prv_art_update);
  layer_add_child(window_layer, s_art_layer);

  prv_update_date();

  s_matchbox_layer = layer_create(GRect(4, 126, 136, 38));
  layer_set_update_proc(s_matchbox_layer, prv_matchbox_update);
  layer_add_child(window_layer, s_matchbox_layer);

  prv_update_time();
  prv_refresh_display();
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  layer_destroy(s_status_layer);
  layer_destroy(s_title_layer);
  layer_destroy(s_art_layer);
  layer_destroy(s_matchbox_layer);
  gbitmap_destroy(s_ball_small);
  gbitmap_destroy(s_ball_large);
  gbitmap_destroy(s_trophy);
  gbitmap_destroy(s_bt_icon);
  if (s_flag1) { gbitmap_destroy(s_flag1); s_flag1 = NULL; }
  if (s_flag2) { gbitmap_destroy(s_flag2); s_flag2 = NULL; }
}

static void prv_refresh_display(void) {
  time_t now = prv_now();
  s_display_match = match_store_display_at(now, s_browse_offset);
  if (!s_display_match && s_browse_offset > 0) {
    /* queue shrank mid-browse: fall back to the soonest game */
    s_browse_offset = 0;
    s_display_match = match_store_display(now);
  }
  if (s_display_match) {
    match_format_teams(s_display_match, s_teams_buf, sizeof(s_teams_buf));
    match_format_when(s_display_match, now, s_when_buf, sizeof(s_when_buf));
    s_flag1 = prv_load_flag(s_display_match->code1, s_flag1, s_flag1_code);
    s_flag2 = prv_load_flag(s_display_match->code2, s_flag2, s_flag2_code);
  } else if (match_store_count() > 0) {
    /* queue non-empty but exhausted: ask the phone for fresh data */
    prv_maybe_request_refresh(now);
  }
  if (s_matchbox_layer) layer_mark_dirty(s_matchbox_layer);
}

static void prv_matchbox_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 3, GCornersAll);
  graphics_context_set_text_color(ctx, GColorWhite);

  if (!s_display_match) {
    graphics_draw_text(ctx, "NO UPCOMING GAMES",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(0, 8, b.size.w, 16),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }

  GRect f1 = GRect(6, 4, 20, 12);
  GRect f2 = GRect(b.size.w - 26, 4, 20, 12);
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  if (s_flag1) graphics_draw_bitmap_in_rect(ctx, s_flag1, f1);
  if (s_flag2) graphics_draw_bitmap_in_rect(ctx, s_flag2, f2);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, grect_inset(f1, GEdgeInsets(-1)));
  graphics_draw_rect(ctx, grect_inset(f2, GEdgeInsets(-1)));

  graphics_draw_text(ctx, s_teams_buf,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(28, 0, b.size.w - 56, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, s_when_buf,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, 17, b.size.w, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  if (s_browse_offset > 0) {
    char marker[12];
    snprintf(marker, sizeof(marker), "+%d", s_browse_offset);
    graphics_draw_text(ctx, marker, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(4, 17, 24, 16),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t = dict_find(iter, MESSAGE_KEY_MATCHES);
  if (!t) return;
  if (match_store_set_payload(t->value->data, t->length)) {
    APP_LOG(APP_LOG_LEVEL_INFO, "received %d matches (%u bytes)",
            match_store_count(), (unsigned)t->length);
    s_browse_offset = 0;  /* list may have reordered; a stale offset lies */
    prv_refresh_display();
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "malformed match payload (%u bytes) - keeping previous", (unsigned)t->length);
  }
}

static void prv_init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorWhite);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  match_store_init();
  app_message_register_inbox_received(prv_inbox_received);
  app_message_open(4096, 64);

  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
  battery_state_service_subscribe(prv_battery_handler);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = prv_connection_handler,
  });
  accel_tap_service_subscribe(prv_tap_handler);
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  accel_tap_service_unsubscribe();
  if (s_browse_timer) app_timer_cancel(s_browse_timer);
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
