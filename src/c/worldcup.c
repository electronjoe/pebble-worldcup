#include <pebble.h>
#include "match_store.h"

static Window *s_window;
static TextLayer *s_time_layer;
static char s_time_buf[8];

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

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  prv_update_time();
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

  prv_update_time();
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
}

static void prv_refresh_display(void) {
  /* Replaced with real UI updates in the match-box task; log for now. */
  time_t now = time(NULL);
  const Match *m = match_store_display(now);
  if (m) {
    char teams[16], when[24];
    match_format_teams(m, teams, sizeof(teams));
    match_format_when(m, now, when, sizeof(when));
    APP_LOG(APP_LOG_LEVEL_INFO, "display match: %s | %s", teams, when);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "display match: none (count=%d)", match_store_count());
  }
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
