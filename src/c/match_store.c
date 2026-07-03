#include "match_store.h"

#include <ctype.h>
#include <string.h>

#define PERSIST_KEY_PAYLOAD_LEN 1
#define PERSIST_KEY_CHUNK_BASE 100
#define MAX_PAYLOAD_BYTES (1 + MATCH_STORE_MAX * MATCH_RECORD_BYTES)

static Match s_matches[MATCH_STORE_MAX];
static int s_count = 0;

static bool prv_validate(const uint8_t *data, uint16_t length) {
  if (length < 1) return false;
  uint8_t count = data[0];
  if (count == 0) return false;  /* empty payload: keep previous queue */
  if (count > MATCH_STORE_MAX) count = MATCH_STORE_MAX;
  return length >= (uint16_t)(1 + count * MATCH_RECORD_BYTES);
}

static void prv_decode(const uint8_t *data) {
  int count = data[0];
  if (count > MATCH_STORE_MAX) count = MATCH_STORE_MAX;
  for (int i = 0; i < count; i++) {
    const uint8_t *rec = data + 1 + i * MATCH_RECORD_BYTES;
    s_matches[i].kickoff = (time_t)((uint32_t)rec[0] | ((uint32_t)rec[1] << 8) |
                                    ((uint32_t)rec[2] << 16) | ((uint32_t)rec[3] << 24));
    memcpy(s_matches[i].code1, rec + 4, 3);
    s_matches[i].code1[3] = '\0';
    memcpy(s_matches[i].code2, rec + 7, 3);
    s_matches[i].code2[3] = '\0';
  }
  s_count = count;
}

static void prv_persist(const uint8_t *data, uint16_t length) {
  persist_write_int(PERSIST_KEY_PAYLOAD_LEN, length);
  uint32_t key = PERSIST_KEY_CHUNK_BASE;
  for (uint16_t off = 0; off < length; off += PERSIST_DATA_MAX_LENGTH, key++) {
    uint16_t n = length - off;
    if (n > PERSIST_DATA_MAX_LENGTH) n = PERSIST_DATA_MAX_LENGTH;
    persist_write_data(key, data + off, n);
  }
}

void match_store_init(void) {
  if (!persist_exists(PERSIST_KEY_PAYLOAD_LEN)) return;
  int32_t length = persist_read_int(PERSIST_KEY_PAYLOAD_LEN);
  if (length <= 0 || length > MAX_PAYLOAD_BYTES) return;
  static uint8_t buf[MAX_PAYLOAD_BYTES];
  uint32_t key = PERSIST_KEY_CHUNK_BASE;
  for (int32_t off = 0; off < length; off += PERSIST_DATA_MAX_LENGTH, key++) {
    int32_t n = length - off;
    if (n > PERSIST_DATA_MAX_LENGTH) n = PERSIST_DATA_MAX_LENGTH;
    if (persist_read_data(key, buf + off, n) != n) return;
  }
  if (prv_validate(buf, length)) {
    prv_decode(buf);
    APP_LOG(APP_LOG_LEVEL_INFO, "match_store: loaded %d matches from persist", s_count);
  }
}

bool match_store_set_payload(const uint8_t *data, uint16_t length) {
  if (length > MAX_PAYLOAD_BYTES || !prv_validate(data, length)) return false;
  prv_decode(data);
  prv_persist(data, length);
  return true;
}

int match_store_count(void) {
  return s_count;
}

const Match *match_store_display(time_t now) {
  for (int i = 0; i < s_count; i++) {
    if (now < s_matches[i].kickoff + MATCH_DISPLAY_WINDOW_SEC) return &s_matches[i];
  }
  return NULL;
}

void match_format_teams(const Match *m, char *buf, size_t buflen) {
  snprintf(buf, buflen, "%s VS %s", m->code1, m->code2);
}

void match_format_when(const Match *m, time_t now, char *buf, size_t buflen) {
  if (now >= m->kickoff) {
    snprintf(buf, buflen, "NOW");
    return;
  }
  struct tm kt = *localtime(&m->kickoff);  /* watch TZ is synced by the system */
  struct tm nt = *localtime(&now);
  struct tm kt0 = kt, nt0 = nt;
  kt0.tm_hour = kt0.tm_min = kt0.tm_sec = 0;
  nt0.tm_hour = nt0.tm_min = nt0.tm_sec = 0;
  int days = (int)((mktime(&kt0) - mktime(&nt0)) / 86400);

  char prefix[12];
  if (days == 0) {
    strcpy(prefix, "TODAY");
  } else if (days == 1) {
    strcpy(prefix, "TOMORROW");
  } else {
    strftime(prefix, sizeof(prefix), "%a", &kt);  /* kickoffs are within a week */
    for (char *p = prefix; *p; p++) *p = toupper((unsigned char)*p);
  }

  if (clock_is_24h_style()) {
    snprintf(buf, buflen, "%s %d:%02d", prefix, kt.tm_hour, kt.tm_min);
  } else {
    int h = kt.tm_hour % 12;
    if (h == 0) h = 12;
    snprintf(buf, buflen, "%s %d:%02d %s", prefix, h, kt.tm_min,
             kt.tm_hour < 12 ? "AM" : "PM");
  }
}
