#include <pebble.h>
#include "habit_icons.h"

#define MAX_HABITS 4
#define ICON_SLUG_LEN 16
#define COLOR_SLUG_LEN 12
#define TODO_MAX_LEN 80

#define HABIT_CORNER_INSET 8
#define EDGE_INSET 12
#define HABIT_CHIP_RADIUS 16
#define DATE_BLOB_W 100
#define DATE_BLOB_H 52
#define TODO_FOOTER_HEIGHT 52
#define TODO_TICK_GAP 11
#define TODO_HABIT_GAP 4
#define TODO_CORNER_RADIUS (TODO_FOOTER_HEIGHT / 2)
#define TODO_TEXT_MAX_LINES 3
#define TODO_TEXT_LINE_HEIGHT 16
#define TODO_TEXT_PAD_X 8
#define TODO_TEXT_PAD_Y 4

#define TICK_LEN_HOUR 7
#define TICK_LEN_QUARTER 12
#define HAND_MINUTE_RATIO 75
#define HAND_HOUR_RATIO 55
#define HAND_HUB_RADIUS 3

#define HEY_RGB(r, g, b) GColorFromRGB((r), (g), (b))

typedef struct {
  GColor bg;
  GColor ink;
  GColor date_splash;
  GColor date_splash_ink;
  GColor todo_pill;
  GColor todo_pill_ink;
  GColor tick_normal;
  GColor tick_blue;
  GColor tick_purple;
  GColor tick_green;
  GColor hand_hour;
  GColor hand_minute;
  GColor hand_hub;
} HeyTheme;

typedef struct {
  GPoint clock_center;
  int clock_radius;
  GPoint habit_corners[4];
  int date_blob_y;
  int todo_y;
  int todo_height;
  int todo_margin;
} Layout;

static Window *s_window;
static Layer *s_canvas_layer;
static GBitmap *s_date_splash_bitmap = NULL;

static Layout s_layout;

static char s_date_month[8];
static char s_date_dayline[12];

static uint8_t s_habit_count = 0;
static uint8_t s_habit_done_mask = 0;
static char s_habit_icons[MAX_HABITS][ICON_SLUG_LEN];
static char s_habit_colors[MAX_HABITS][COLOR_SLUG_LEN];
static char s_todo_text[TODO_MAX_LEN + 1];
static char s_todo_display[TODO_MAX_LEN + 8];
static char s_todo_color[COLOR_SLUG_LEN] = "blue";
static uint8_t s_footer_kind = 0;
static uint8_t s_sync_status = 0;
static uint8_t s_theme_mode = 0;
static HeyTheme s_theme;

static GFont s_todo_font;
static int s_todo_text_w;
static int s_todo_draw_h;
static bool s_todo_layout_valid;

typedef struct {
  GBitmap *bitmap;
  char slug[ICON_SLUG_LEN];
  bool white;
  bool valid;
} HabitIconCacheEntry;

static HabitIconCacheEntry s_habit_icon_cache[MAX_HABITS];

static void apply_theme(uint8_t mode) {
  s_theme_mode = mode ? 1 : 0;
  if (s_theme_mode) {
    s_theme = (HeyTheme) {
      .bg = HEY_RGB(27, 39, 51),
      .ink = HEY_RGB(236, 233, 230),
      .date_splash = HEY_RGB(255, 155, 88),
      .date_splash_ink = HEY_RGB(35, 28, 51),
      .todo_pill = HEY_RGB(58, 58, 64),
      .todo_pill_ink = HEY_RGB(236, 233, 230),
      .tick_normal = HEY_RGB(80, 88, 98),
      .tick_blue = HEY_RGB(80, 162, 255),
      .tick_purple = HEY_RGB(134, 126, 255),
      .tick_green = HEY_RGB(105, 240, 174),
      .hand_hour = HEY_RGB(134, 126, 255),
      .hand_minute = HEY_RGB(80, 162, 255),
      .hand_hub = HEY_RGB(134, 126, 255),
    };
  } else {
    s_theme = (HeyTheme) {
      .bg = HEY_RGB(250, 248, 245),
      .ink = HEY_RGB(35, 28, 51),
      .date_splash = HEY_RGB(255, 138, 50),
      .date_splash_ink = HEY_RGB(255, 255, 255),
      .todo_pill = HEY_RGB(168, 168, 255),
      .todo_pill_ink = HEY_RGB(35, 28, 51),
      .tick_normal = HEY_RGB(170, 170, 178),
      .tick_blue = HEY_RGB(0, 116, 228),
      .tick_purple = HEY_RGB(85, 34, 250),
      .tick_green = HEY_RGB(41, 152, 80),
      .hand_hour = HEY_RGB(85, 34, 250),
      .hand_minute = HEY_RGB(0, 116, 228),
      .hand_hub = HEY_RGB(85, 34, 250),
    };
  }

  if (s_window) {
    window_set_background_color(s_window, s_theme.bg);
  }
}

static int clock_radius_for_bounds(GRect bounds, GPoint center) {
  int r_top = center.y - EDGE_INSET;
  int r_bottom = bounds.size.h - EDGE_INSET - center.y;
  int r_left = center.x - EDGE_INSET;
  int r_right = bounds.size.w - EDGE_INSET - center.x;
  int r = r_top;
  if (r_bottom < r) {
    r = r_bottom;
  }
  if (r_left < r) {
    r = r_left;
  }
  if (r_right < r) {
    r = r_right;
  }
  return r;
}

static void compute_layout(GRect bounds) {
  int habit_corner = HABIT_CORNER_INSET + HABIT_CHIP_RADIUS;

  s_layout.clock_center = grect_center_point(&bounds);
  s_layout.clock_radius = clock_radius_for_bounds(bounds, s_layout.clock_center);
  s_layout.habit_corners[0] = GPoint(habit_corner, habit_corner);
  s_layout.habit_corners[1] = GPoint(bounds.size.w - habit_corner, habit_corner);
  s_layout.habit_corners[2] = GPoint(habit_corner, bounds.size.h - habit_corner);
  s_layout.habit_corners[3] = GPoint(bounds.size.w - habit_corner, bounds.size.h - habit_corner);
  s_layout.todo_height = TODO_FOOTER_HEIGHT;
  s_layout.todo_margin = habit_corner + HABIT_CHIP_RADIUS + TODO_HABIT_GAP;

  int tick_inner_top = s_layout.clock_center.y - s_layout.clock_radius + TICK_LEN_QUARTER;
  int date_mid_y = (s_layout.clock_center.y + tick_inner_top) / 2;
  s_layout.date_blob_y = date_mid_y - DATE_BLOB_H / 2;

  int tick_inner_bottom = s_layout.clock_center.y + s_layout.clock_radius - TICK_LEN_QUARTER;
  s_layout.todo_y = tick_inner_bottom - s_layout.todo_height - TODO_TICK_GAP;
}

static GPoint habit_chip_center(int index) {
  if (index >= 0 && index < MAX_HABITS) {
    return s_layout.habit_corners[index];
  }
  return s_layout.clock_center;
}

static void hey_color_rgb(const char *slug, uint8_t *r, uint8_t *g, uint8_t *b) {
  if (s_theme_mode) {
    if (strcmp(slug, "green") == 0) { *r = 105; *g = 240; *b = 174; return; }
    if (strcmp(slug, "red") == 0) { *r = 255; *g = 120; *b = 120; return; }
    if (strcmp(slug, "orange") == 0) { *r = 255; *g = 184; *b = 92; return; }
    if (strcmp(slug, "gold") == 0) { *r = 249; *g = 213; *b = 122; return; }
    if (strcmp(slug, "yellow") == 0) { *r = 251; *g = 225; *b = 144; return; }
    if (strcmp(slug, "purple") == 0) { *r = 134; *g = 126; *b = 255; return; }
    if (strcmp(slug, "pink") == 0) { *r = 242; *g = 136; *b = 187; return; }
    if (strcmp(slug, "teal") == 0) { *r = 157; *g = 255; *b = 236; return; }
    if (strcmp(slug, "brown") == 0) { *r = 185; *g = 165; *b = 151; return; }
    *r = 80; *g = 162; *b = 255;
    return;
  }
  if (strcmp(slug, "green") == 0) { *r = 41; *g = 152; *b = 80; return; }
  if (strcmp(slug, "red") == 0) { *r = 201; *g = 36; *b = 0; return; }
  if (strcmp(slug, "orange") == 0) { *r = 248; *g = 121; *b = 23; return; }
  if (strcmp(slug, "gold") == 0) { *r = 166; *g = 119; *b = 0; return; }
  if (strcmp(slug, "yellow") == 0) { *r = 255; *g = 214; *b = 10; return; }
  if (strcmp(slug, "purple") == 0) { *r = 85; *g = 34; *b = 250; return; }
  if (strcmp(slug, "pink") == 0) { *r = 179; *g = 25; *b = 99; return; }
  if (strcmp(slug, "teal") == 0) { *r = 19; *g = 142; *b = 158; return; }
  if (strcmp(slug, "brown") == 0) { *r = 119; *g = 109; *b = 99; return; }
  *r = 0; *g = 116; *b = 228;
}

static GColor hey_color_fill(const char *slug) {
  uint8_t r, g, b;
  hey_color_rgb(slug, &r, &g, &b);
  return HEY_RGB(r, g, b);
}

static GColor quarter_tick_color(int hour) {
  if (hour == 0) {
    return s_theme.tick_blue;
  }
  if (hour == 6) {
    return s_theme.tick_green;
  }
  if (hour == 3 || hour == 9) {
    return s_theme.tick_purple;
  }
  return s_theme.tick_normal;
}

static void draw_hour_ticks(GContext *ctx) {
  GPoint center = s_layout.clock_center;
  int radius = s_layout.clock_radius;

  for (int hour = 0; hour < 12; hour++) {
    int32_t angle = hour * TRIG_MAX_ANGLE / 12;
    bool is_quarter = (hour % 3) == 0;
    int tick_len = is_quarter ? TICK_LEN_QUARTER : TICK_LEN_HOUR;
    int outer_r = radius;
    int inner_r = outer_r - tick_len;
    int stroke = is_quarter ? 2 : 1;
    GColor color = is_quarter ? quarter_tick_color(hour) : s_theme.tick_normal;

    GPoint outer = {
      center.x + (sin_lookup(angle) * outer_r) / TRIG_MAX_RATIO,
      center.y - (cos_lookup(angle) * outer_r) / TRIG_MAX_RATIO
    };
    GPoint inner = {
      center.x + (sin_lookup(angle) * inner_r) / TRIG_MAX_RATIO,
      center.y - (cos_lookup(angle) * inner_r) / TRIG_MAX_RATIO
    };

    graphics_context_set_stroke_color(ctx, color);
    graphics_context_set_stroke_width(ctx, stroke);
    graphics_draw_line(ctx, outer, inner);
  }
}

static void draw_clock_hands(GContext *ctx, struct tm *tick_time) {
  GPoint center = s_layout.clock_center;
  int hour_len = (s_layout.clock_radius * HAND_HOUR_RATIO) / 100;
  int minute_len = (s_layout.clock_radius * HAND_MINUTE_RATIO) / 100;

  int32_t hour_angle = ((tick_time->tm_hour % 12) * TRIG_MAX_ANGLE / 12) +
                       (tick_time->tm_min * TRIG_MAX_ANGLE / 12 / 60);
  int32_t minute_angle = tick_time->tm_min * TRIG_MAX_ANGLE / 60;

  GPoint hour_end = {
    center.x + (sin_lookup(hour_angle) * hour_len) / TRIG_MAX_RATIO,
    center.y - (cos_lookup(hour_angle) * hour_len) / TRIG_MAX_RATIO
  };
  GPoint minute_end = {
    center.x + (sin_lookup(minute_angle) * minute_len) / TRIG_MAX_RATIO,
    center.y - (cos_lookup(minute_angle) * minute_len) / TRIG_MAX_RATIO
  };

  graphics_context_set_stroke_color(ctx, s_theme.hand_hour);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, center, hour_end);

  graphics_context_set_stroke_color(ctx, s_theme.hand_minute);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, center, minute_end);

  graphics_context_set_fill_color(ctx, s_theme.hand_hub);
  graphics_fill_circle(ctx, center, HAND_HUB_RADIUS);
  graphics_context_set_stroke_color(ctx, s_theme.hand_minute);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, center, HAND_HUB_RADIUS);
}

static const char *habit_icon_slug_alias(const char *slug) {
  if (strcmp(slug, "book") == 0) return "read";
  if (strcmp(slug, "bike") == 0) return "bicycle";
  if (strcmp(slug, "water") == 0) return "hydrate";
  if (strcmp(slug, "drop") == 0) return "hydrate";
  if (strcmp(slug, "sleep") == 0) return "bed";
  if (strcmp(slug, "food") == 0) return "cook";
  if (strcmp(slug, "pray") == 0) return "church";
  if (strcmp(slug, "love") == 0) return "heart";
  if (strcmp(slug, "fitness") == 0) return "weights";
  if (strcmp(slug, "kettlebells") == 0) return "weights";
  return slug;
}

static void clear_habit_icon_cache(void) {
  for (int i = 0; i < MAX_HABITS; i++) {
    if (s_habit_icon_cache[i].bitmap) {
      gbitmap_destroy(s_habit_icon_cache[i].bitmap);
    }
    s_habit_icon_cache[i].bitmap = NULL;
    s_habit_icon_cache[i].valid = false;
    s_habit_icon_cache[i].slug[0] = '\0';
  }
}

static GBitmap *habit_icon_cached(int slot, const char *icon_slug, bool white) {
  HabitIconCacheEntry *entry = &s_habit_icon_cache[slot];

  if (entry->valid && entry->white == white &&
      strcmp(entry->slug, icon_slug) == 0) {
    return entry->bitmap;
  }

  if (entry->bitmap) {
    gbitmap_destroy(entry->bitmap);
    entry->bitmap = NULL;
    entry->valid = false;
  }

  entry->bitmap = gbitmap_create_with_resource(
      habit_icon_resource_for_slug(icon_slug, white));
  strncpy(entry->slug, icon_slug, ICON_SLUG_LEN - 1);
  entry->slug[ICON_SLUG_LEN - 1] = '\0';
  entry->white = white;
  entry->valid = entry->bitmap != NULL;
  return entry->bitmap;
}

static void draw_habit_icon_native(GContext *ctx, GPoint center, GBitmap *icon) {
  GRect bounds = gbitmap_get_bounds(icon);
  GRect dest = GRect(center.x - bounds.size.w / 2,
                     center.y - bounds.size.h / 2,
                     bounds.size.w, bounds.size.h);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, icon, dest);
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
}

static void draw_habit_glyph(GContext *ctx, int slot, GPoint center, bool done,
                             GColor color, const char *slug) {
  const char *icon_slug = habit_icon_slug_alias(slug);
  GBitmap *icon;
  bool use_white_glyph;

  if (done) {
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_circle(ctx, center, HABIT_CHIP_RADIUS);
    use_white_glyph = !s_theme_mode;
  } else {
    graphics_context_set_stroke_color(ctx, color);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, center, HABIT_CHIP_RADIUS);
    use_white_glyph = s_theme_mode;
  }

  icon = habit_icon_cached(slot, icon_slug, use_white_glyph);
  if (icon) {
    draw_habit_icon_native(ctx, center, icon);
  }
}

static void draw_habits(GContext *ctx) {
  for (int i = 0; i < s_habit_count && i < MAX_HABITS; i++) {
    if (s_habit_icons[i][0] == '\0') {
      continue;
    }
    bool done = (s_habit_done_mask & (1 << i)) != 0;
    GColor color = hey_color_fill(s_habit_colors[i]);
    draw_habit_glyph(ctx, i, habit_chip_center(i), done, color, s_habit_icons[i]);
  }
}

static void draw_date_splash(GContext *ctx, GRect bounds) {
  if (!s_date_splash_bitmap) {
    return;
  }

  int blob_x = (bounds.size.w - DATE_BLOB_W) / 2;
  GRect blob = GRect(blob_x, s_layout.date_blob_y, DATE_BLOB_W, DATE_BLOB_H);

  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_date_splash_bitmap, blob);
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);

  graphics_context_set_text_color(ctx, s_theme.date_splash_ink);
  GRect month_box = GRect(blob.origin.x, blob.origin.y + 4, blob.size.w, 16);
  GRect day_box = GRect(blob.origin.x, blob.origin.y + 20, blob.size.w, 22);
  graphics_draw_text(ctx, s_date_month, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     month_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, false);
  graphics_draw_text(ctx, s_date_dayline, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     day_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, false);
}

static int todo_text_line_height(GFont font, int text_w) {
  GRect box = GRect(0, 0, text_w, 200);
  GSize one = graphics_text_layout_get_content_size(
      "A", font, box, GTextOverflowModeWordWrap, GTextAlignmentCenter);
  GSize two = graphics_text_layout_get_content_size(
      "A\nB", font, box, GTextOverflowModeWordWrap, GTextAlignmentCenter);
  int line_h = (int) two.h - (int) one.h;
  if (line_h < 8 && one.h > 0) {
    line_h = (int) one.h;
  }
  if (line_h < 8) {
    line_h = TODO_TEXT_LINE_HEIGHT;
  }
  return line_h;
}

static void prepare_todo_display(GFont font, int text_w, int max_h) {
  GRect layout_box = GRect(0, 0, text_w, max_h);
  GSize full_size = graphics_text_layout_get_content_size(
      s_todo_text, font, layout_box, GTextOverflowModeWordWrap, GTextAlignmentCenter);
  if (full_size.h <= max_h) {
    snprintf(s_todo_display, sizeof(s_todo_display), "%s", s_todo_text);
    return;
  }

  int lo = 0;
  int hi = (int) strlen(s_todo_text);
  while (lo < hi) {
    int mid = (lo + hi + 1) / 2;
    snprintf(s_todo_display, sizeof(s_todo_display), "%.*s...", mid, s_todo_text);
    GSize size = graphics_text_layout_get_content_size(
        s_todo_display, font, layout_box, GTextOverflowModeWordWrap, GTextAlignmentCenter);
    if (size.h <= max_h) {
      lo = mid;
    } else {
      hi = mid - 1;
    }
  }

  snprintf(s_todo_display, sizeof(s_todo_display), "%.*s...", lo, s_todo_text);
}

static void rebuild_todo_layout(int bar_w) {
  s_todo_text_w = bar_w - TODO_TEXT_PAD_X * 2;
  if (s_todo_text[0] == '\0' || s_todo_text_w <= 0) {
    s_todo_layout_valid = false;
    return;
  }

  int line_h = todo_text_line_height(s_todo_font, s_todo_text_w);
  int max_text_h = s_layout.todo_height - TODO_TEXT_PAD_Y * 2;
  if (max_text_h > TODO_TEXT_MAX_LINES * line_h) {
    max_text_h = TODO_TEXT_MAX_LINES * line_h;
  }
  prepare_todo_display(s_todo_font, s_todo_text_w, max_text_h);

  GSize text_size = graphics_text_layout_get_content_size(
      s_todo_display, s_todo_font, GRect(0, 0, s_todo_text_w, max_text_h),
      GTextOverflowModeWordWrap, GTextAlignmentCenter);
  s_todo_draw_h = (int) text_size.h;
  if (s_todo_draw_h > max_text_h) {
    s_todo_draw_h = max_text_h;
  }
  s_todo_layout_valid = true;
}

static void draw_todo_footer(GContext *ctx, GRect bounds) {
  if (s_todo_text[0] == '\0' || !s_todo_layout_valid) {
    return;
  }

  int bar_x = s_layout.todo_margin;
  int bar_w = bounds.size.w - s_layout.todo_margin * 2;
  GRect bar = GRect(bar_x, s_layout.todo_y, bar_w, s_layout.todo_height);

  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  graphics_context_set_fill_color(ctx, s_theme.todo_pill);
  graphics_fill_rect(ctx, bar, TODO_CORNER_RADIUS, GCornersAll);

  graphics_context_set_text_color(ctx, s_theme.todo_pill_ink);
  int text_y = bar.origin.y + (bar.size.h - s_todo_draw_h) / 2;
  GRect text_box = GRect(bar.origin.x + TODO_TEXT_PAD_X, text_y, s_todo_text_w, s_todo_draw_h);
  graphics_draw_text(ctx, s_todo_display, s_todo_font, text_box,
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, s_theme.bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_habits(ctx);
  draw_date_splash(ctx, bounds);
  draw_hour_ticks(ctx);
  draw_todo_footer(ctx, bounds);

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  if (tick_time) {
    draw_clock_hands(ctx, tick_time);
  }
}

static void update_date(void) {
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  if (!tick_time) return;

  static char weekday_buffer[8];
  strftime(weekday_buffer, sizeof(weekday_buffer), "%a", tick_time);
  strftime(s_date_month, sizeof(s_date_month), "%b", tick_time);
  snprintf(s_date_dayline, sizeof(s_date_dayline), "%s %d",
           weekday_buffer, tick_time->tm_mday);
}

static void request_hey_data(void) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) {
    return;
  }

  static char date_buffer[11];
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  if (tick_time) {
    strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", tick_time);
    dict_write_cstring(iter, MESSAGE_KEY_QUERY_DATE, date_buffer);
  }

  dict_write_uint8(iter, MESSAGE_KEY_REQUEST_HEY_DATA, 1);
  app_message_outbox_send();
}

static void parse_pipe_field(char *dest, int dest_len, const char *src, int index) {
  dest[0] = '\0';
  if (!src || src[0] == '\0') return;

  int current = 0;
  int out = 0;
  for (int i = 0; src[i] != '\0' && out < dest_len - 1; i++) {
    if (src[i] == '|') {
      if (current == index) {
        dest[out] = '\0';
        return;
      }
      current++;
      out = 0;
      dest[0] = '\0';
      continue;
    }
    if (current == index) {
      dest[out++] = src[i];
    }
  }
  if (current == index) {
    dest[out] = '\0';
  }
}

static bool habit_data_changed(uint8_t count, uint8_t mask, const char *icons, const char *colors) {
  if (count != s_habit_count || mask != s_habit_done_mask) {
    return true;
  }

  char tmp_icon[ICON_SLUG_LEN];
  char tmp_color[COLOR_SLUG_LEN];
  for (int i = 0; i < count && i < MAX_HABITS; i++) {
    parse_pipe_field(tmp_icon, sizeof(tmp_icon), icons ? icons : "", i);
    parse_pipe_field(tmp_color, sizeof(tmp_color), colors ? colors : "", i);
    if (strcmp(tmp_icon, s_habit_icons[i]) != 0) return true;
    if (strcmp(tmp_color, s_habit_colors[i]) != 0) return true;
  }
  return false;
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *count_tuple = dict_find(iterator, MESSAGE_KEY_HABIT_COUNT);
  Tuple *mask_tuple = dict_find(iterator, MESSAGE_KEY_HABIT_DONE_MASK);
  Tuple *icons_tuple = dict_find(iterator, MESSAGE_KEY_HABIT_ICONS);
  Tuple *colors_tuple = dict_find(iterator, MESSAGE_KEY_HABIT_COLORS);
  Tuple *todo_tuple = dict_find(iterator, MESSAGE_KEY_TODO_TEXT);
  Tuple *todo_color_tuple = dict_find(iterator, MESSAGE_KEY_TODO_COLOR);
  Tuple *footer_kind_tuple = dict_find(iterator, MESSAGE_KEY_FOOTER_KIND);
  Tuple *status_tuple = dict_find(iterator, MESSAGE_KEY_SYNC_STATUS);
  Tuple *theme_tuple = dict_find(iterator, MESSAGE_KEY_THEME_MODE);

  bool changed = false;
  bool footer_changed = false;
  bool clear_icons = false;

  if (theme_tuple) {
    uint8_t mode = theme_tuple->value->uint8 ? 1 : 0;
    if (mode != s_theme_mode) {
      apply_theme(mode);
      clear_icons = true;
      footer_changed = true;
      changed = true;
    }
  }

  if (status_tuple) {
    s_sync_status = status_tuple->value->uint8;
  }

  if (count_tuple) {
    uint8_t count = count_tuple->value->uint8;
    uint8_t mask = mask_tuple ? mask_tuple->value->uint8 : 0;
    const char *icons = icons_tuple ? icons_tuple->value->cstring : "";
    const char *colors = colors_tuple ? colors_tuple->value->cstring : "";

    if (habit_data_changed(count, mask, icons, colors)) {
      clear_icons = true;
      s_habit_count = count;
      s_habit_done_mask = mask;
      for (int i = 0; i < MAX_HABITS; i++) {
        s_habit_icons[i][0] = '\0';
        s_habit_colors[i][0] = '\0';
      }
      for (int i = 0; i < MAX_HABITS; i++) {
        parse_pipe_field(s_habit_icons[i], ICON_SLUG_LEN, icons, i);
        parse_pipe_field(s_habit_colors[i], COLOR_SLUG_LEN, colors, i);
      }
      changed = true;
    }
  }

  if (clear_icons) {
    clear_habit_icon_cache();
  }

  if (todo_tuple) {
    const char *todo = todo_tuple->value->cstring;
    if (strcmp(todo, s_todo_text) != 0) {
      snprintf(s_todo_text, sizeof(s_todo_text), "%s", todo);
      footer_changed = true;
      changed = true;
    }
  }

  if (todo_color_tuple) {
    const char *color = todo_color_tuple->value->cstring;
    if (strcmp(color, s_todo_color) != 0) {
      snprintf(s_todo_color, sizeof(s_todo_color), "%s", color);
    }
  }

  if (footer_kind_tuple) {
    uint8_t kind = footer_kind_tuple->value->uint8;
    if (kind != s_footer_kind) {
      s_footer_kind = kind;
    }
  }

  if (footer_changed && s_canvas_layer) {
    int bar_w = layer_get_bounds(s_canvas_layer).size.w - s_layout.todo_margin * 2;
    rebuild_todo_layout(bar_w);
  }

  if (changed) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & DAY_UNIT) {
    update_date();
    layer_mark_dirty(s_canvas_layer);
  } else if (units_changed & MINUTE_UNIT) {
    request_hey_data();
    layer_mark_dirty(s_canvas_layer);
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  compute_layout(bounds);

  s_todo_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  s_date_splash_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DATE_SPLASH);

  update_date();
  rebuild_todo_layout(bounds.size.w - s_layout.todo_margin * 2);
}

static void window_unload(Window *window) {
  clear_habit_icon_cache();
  if (s_date_splash_bitmap) {
    gbitmap_destroy(s_date_splash_bitmap);
    s_date_splash_bitmap = NULL;
  }
  layer_destroy(s_canvas_layer);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  (void) iterator;
  (void) context;
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  (void) reason;
  (void) context;
}

static void init(void) {
  apply_theme(0);
  s_window = window_create();
  window_set_background_color(s_window, s_theme.bg);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_open(2048, 1024);

  tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, tick_handler);

  request_hey_data();
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
