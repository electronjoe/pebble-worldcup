#include <pebble.h>
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

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  prv_update_time();
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

  s_matchbox_layer = layer_create(GRect(4, 126, 136, 38));
  layer_set_update_proc(s_matchbox_layer, prv_matchbox_update);
  layer_add_child(window_layer, s_matchbox_layer);

  prv_update_time();
  prv_refresh_display();
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  layer_destroy(s_matchbox_layer);
  if (s_flag1) { gbitmap_destroy(s_flag1); s_flag1 = NULL; }
  if (s_flag2) { gbitmap_destroy(s_flag2); s_flag2 = NULL; }
}

static void prv_refresh_display(void) {
  time_t now = prv_now();
  s_display_match = match_store_display(now);
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
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t = dict_find(iter, MESSAGE_KEY_MATCHES);
  if (!t) return;
  if (match_store_set_payload(t->value->data, t->length)) {
    APP_LOG(APP_LOG_LEVEL_INFO, "received %d matches (%u bytes)",
            match_store_count(), (unsigned)t->length);
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
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
