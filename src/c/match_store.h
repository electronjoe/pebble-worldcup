#pragma once
#include <pebble.h>

#define MATCH_STORE_MAX 128
#define MATCH_RECORD_BYTES 10
#define MATCH_DISPLAY_WINDOW_SEC 9000  /* 2.5 hours */

typedef struct {
  time_t kickoff;
  char code1[4];
  char code2[4];
} Match;

void match_store_init(void);
bool match_store_set_payload(const uint8_t *data, uint16_t length);
int match_store_count(void);
const Match *match_store_display(time_t now);
const Match *match_store_display_at(time_t now, int offset);
int match_store_upcoming_count(time_t now);
void match_format_teams(const Match *m, char *buf, size_t buflen);
void match_format_when(const Match *m, time_t now, char *buf, size_t buflen);
