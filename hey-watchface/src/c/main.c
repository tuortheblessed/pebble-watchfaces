#include <pebble.h>
#include "habit_icons.h"

#define MAX_HABITS 4
#define ICON_SLUG_LEN 16
#define COLOR_SLUG_LEN 12
#define TODO_MAX_LEN 80
#define REFRESH_INTERVAL_MS 30000

#define PERIMETER_INSET 12
#define HABIT_CHIP_RADIUS 16
#define HABIT_CHIP_SIZE (HABIT_CHIP_RADIUS * 2)
#define INNER_INSET 12
#define TODO_FOOTER_HEIGHT 30
#define TODO_FOOTER_MARGIN 4
#define TODO_BELOW_HABITS_GAP 10
#define FOOTER_TICK_CLEARANCE (PERIMETER_INSET + TICK_LEN_HOUR + 8)

#define HEY_RGB(r, g, b) GColorFromRGB((r), (g), (b))

typedef struct {
  GColor bg;
  GColor ink;
  GColor text_secondary;
  GColor chip_bg;
  GColor tick_normal;
  GColor tick_hour;
  GColor tick_minute_accent;
  GColor tick_hour_accent;
  GColor frame;
  GColor todo_bar;
} HeyTheme;

#define TICK_LEN_NORMAL 5
#define TICK_LEN_HOUR 11
#define TICK_LEN_CURRENT_MIN 15
#define TICK_LEN_CURRENT_HOUR 20

typedef struct {
  GRect inner;
  GRect tick_bounds;
  int habit_center_y;
  int date_center_y;
  int habit_offset_x;
  int habit_offset_y;
  int todo_y;
  int todo_height;
  int todo_margin;
} Layout;

static Window *s_window;
static Layer *s_canvas_layer;
static TextLayer *s_date_weekday_layer;
static TextLayer *s_date_day_layer;

static Layout s_layout;
static GPoint s_center;
static int s_current_minute = -1;
static int s_current_hour_tick = -1;

static uint8_t s_habit_count = 0;
static uint8_t s_habit_done_mask = 0;
static char s_habit_icons[MAX_HABITS][ICON_SLUG_LEN];
static char s_habit_colors[MAX_HABITS][COLOR_SLUG_LEN];
static char s_todo_text[TODO_MAX_LEN + 1];
static uint8_t s_footer_kind = 0;
static uint8_t s_sync_status = 0;
static uint8_t s_theme_mode = 0;
static HeyTheme s_theme;

static AppTimer *s_refresh_timer = NULL;

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
      .text_secondary = HEY_RGB(155, 153, 152),
      .chip_bg = HEY_RGB(27, 39, 51),
      .tick_normal = HEY_RGB(80, 88, 98),
      .tick_hour = HEY_RGB(140, 145, 155),
      .tick_minute_accent = HEY_RGB(80, 162, 255),
      .tick_hour_accent = HEY_RGB(134, 126, 255),
      .frame = HEY_RGB(50, 58, 70),
      .todo_bar = HEY_RGB(32, 44, 56),
    };
  } else {
    s_theme = (HeyTheme) {
      .bg = HEY_RGB(255, 255, 255),
      .ink = HEY_RGB(35, 28, 51),
      .text_secondary = HEY_RGB(116, 116, 128),
      .chip_bg = HEY_RGB(255, 255, 255),
      .tick_normal = HEY_RGB(170, 170, 178),
      .tick_hour = HEY_RGB(60, 60, 72),
      .tick_minute_accent = HEY_RGB(0, 116, 228),
      .tick_hour_accent = HEY_RGB(85, 34, 250),
      .frame = HEY_RGB(228, 228, 232),
      .todo_bar = HEY_RGB(245, 245, 247),
    };
  }

  if (s_window) {
    window_set_background_color(s_window, s_theme.bg);
  }
  if (s_date_weekday_layer) {
    text_layer_set_text_color(s_date_weekday_layer, s_theme.ink);
    text_layer_set_text_color(s_date_day_layer, s_theme.text_secondary);
  }
}

static void compute_layout(GRect bounds) {
  s_layout.inner = GRect(INNER_INSET, INNER_INSET,
                         bounds.size.w - INNER_INSET * 2,
                         bounds.size.h - INNER_INSET * 2);
  s_layout.tick_bounds = bounds;
  s_layout.habit_center_y = s_layout.inner.origin.y + (s_layout.inner.size.h / 2) - 14;
  s_layout.habit_offset_x = (int)(s_layout.inner.size.w / 5.5f);
  s_layout.habit_offset_y = 40;
  s_layout.date_center_y = s_layout.habit_center_y;
  s_layout.todo_height = TODO_FOOTER_HEIGHT;
  s_layout.todo_margin = TODO_FOOTER_MARGIN;

  int bottom_habit_bottom = s_layout.habit_center_y + s_layout.habit_offset_y + (HABIT_CHIP_SIZE / 2);
  s_layout.todo_y = bottom_habit_bottom + TODO_BELOW_HABITS_GAP;

  int max_footer_bottom = bounds.size.h - FOOTER_TICK_CLEARANCE;
  if (s_layout.todo_y + s_layout.todo_height > max_footer_bottom) {
    s_layout.todo_y = max_footer_bottom - s_layout.todo_height;
  }

  s_center = grect_center_point(&bounds);
}

static GPoint habit_chip_center(int index) {
  int cx = s_layout.inner.origin.x + s_layout.inner.size.w / 2;
  int cy = s_layout.habit_center_y;

  switch (index) {
    case 0: return GPoint(cx - s_layout.habit_offset_x, cy - s_layout.habit_offset_y);
    case 1: return GPoint(cx + s_layout.habit_offset_x, cy - s_layout.habit_offset_y);
    case 2: return GPoint(cx - s_layout.habit_offset_x, cy + s_layout.habit_offset_y);
    default: return GPoint(cx + s_layout.habit_offset_x, cy + s_layout.habit_offset_y);
  }
}

static GPoint point_on_rect_perimeter(int dist, GRect bounds) {
  int w = bounds.size.w;
  int h = bounds.size.h;
  int cx = bounds.size.w / 2;
  int perimeter = 2 * (w + h);
  int d = dist % perimeter;
  int half_w = w / 2;

  if (d < half_w) {
    return GPoint(cx + d, PERIMETER_INSET);
  }
  d -= half_w;
  if (d < h) {
    return GPoint(w - 1 - PERIMETER_INSET, d);
  }
  d -= h;
  if (d < w) {
    return GPoint(w - 1 - d, h - 1 - PERIMETER_INSET);
  }
  d -= w;
  if (d < h) {
    return GPoint(PERIMETER_INSET, h - 1 - d);
  }
  d -= h;
  return GPoint(d, PERIMETER_INSET);
}

static void draw_radial_tick(GContext *ctx, int minute, GRect bounds, GPoint center) {
  int perimeter = 2 * (bounds.size.w + bounds.size.h);
  int dist = (minute * perimeter) / 60;
  GPoint outer = point_on_rect_perimeter(dist, bounds);

  int dx = center.x - outer.x;
  int dy = center.y - outer.y;
  int32_t angle = atan2_lookup(dy, dx);

  bool is_hour_mark = (minute % 5) == 0;
  bool is_current_minute = minute == s_current_minute;
  bool is_current_hour = is_hour_mark && minute == s_current_hour_tick;

  int tick_len = TICK_LEN_NORMAL;
  int stroke = 1;
  GColor color = s_theme.tick_normal;

  if (is_current_hour) {
    tick_len = TICK_LEN_CURRENT_HOUR;
    stroke = 4;
    color = s_theme.tick_hour_accent;
  } else if (is_current_minute) {
    tick_len = TICK_LEN_CURRENT_MIN;
    stroke = 3;
    color = s_theme.tick_minute_accent;
  } else if (is_hour_mark) {
    tick_len = TICK_LEN_HOUR;
    stroke = 2;
    color = s_theme.tick_hour;
  }

  GPoint inner = {
    outer.x + (cos_lookup(angle) * tick_len) / TRIG_MAX_RATIO,
    outer.y + (sin_lookup(angle) * tick_len) / TRIG_MAX_RATIO
  };

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, stroke);
  graphics_draw_line(ctx, outer, inner);
}

static void draw_tick_ring(GContext *ctx) {
  for (int minute = 0; minute < 60; minute++) {
    draw_radial_tick(ctx, minute, s_layout.tick_bounds, s_center);
  }
}

static void draw_inner_frame(GContext *ctx) {
  graphics_context_set_stroke_color(ctx, s_theme.frame);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(ctx, s_layout.inner);
}

static GColor hey_color_fill(const char *slug) {
  if (s_theme_mode) {
    if (strcmp(slug, "blue") == 0) return HEY_RGB(80, 162, 255);
    if (strcmp(slug, "green") == 0) return HEY_RGB(105, 240, 174);
    if (strcmp(slug, "red") == 0) return HEY_RGB(255, 120, 120);
    if (strcmp(slug, "orange") == 0) return HEY_RGB(255, 184, 92);
    if (strcmp(slug, "gold") == 0) return HEY_RGB(249, 213, 122);
    if (strcmp(slug, "yellow") == 0) return HEY_RGB(251, 225, 144);
    if (strcmp(slug, "purple") == 0) return HEY_RGB(134, 126, 255);
    if (strcmp(slug, "pink") == 0) return HEY_RGB(242, 136, 187);
    if (strcmp(slug, "teal") == 0) return HEY_RGB(157, 255, 236);
    if (strcmp(slug, "brown") == 0) return HEY_RGB(185, 165, 151);
    return s_theme.ink;
  }
  if (strcmp(slug, "blue") == 0) return HEY_RGB(0, 116, 228);
  if (strcmp(slug, "green") == 0) return HEY_RGB(41, 152, 80);
  if (strcmp(slug, "red") == 0) return HEY_RGB(201, 36, 0);
  if (strcmp(slug, "orange") == 0) return HEY_RGB(248, 121, 23);
  if (strcmp(slug, "gold") == 0) return HEY_RGB(166, 119, 0);
  if (strcmp(slug, "yellow") == 0) return HEY_RGB(255, 214, 10);
  if (strcmp(slug, "purple") == 0) return HEY_RGB(85, 34, 250);
  if (strcmp(slug, "pink") == 0) return HEY_RGB(179, 25, 99);
  if (strcmp(slug, "teal") == 0) return HEY_RGB(19, 142, 158);
  if (strcmp(slug, "brown") == 0) return HEY_RGB(119, 109, 99);
  return s_theme.ink;
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

static GColor habit_ring_color(void) {
  return s_theme_mode ? HEY_RGB(155, 153, 152) : HEY_RGB(154, 154, 160);
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
    graphics_context_set_stroke_color(ctx, habit_ring_color());
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
    bool done = (s_habit_done_mask & (1 << i)) != 0;
    GColor color = hey_color_fill(s_habit_colors[i]);
    draw_habit_glyph(ctx, i, habit_chip_center(i), done, color, s_habit_icons[i]);
  }
}

static void draw_todo_footer(GContext *ctx, GRect bounds) {
  if (s_todo_text[0] == '\0') {
    return;
  }

  int bar_x = s_layout.inner.origin.x + s_layout.todo_margin;
  int bar_w = s_layout.inner.size.w - s_layout.todo_margin * 2;
  GRect bar = GRect(bar_x, s_layout.todo_y, bar_w, s_layout.todo_height);
  graphics_context_set_fill_color(ctx, s_theme.todo_bar);
  graphics_fill_rect(ctx, bar, 4, GCornersAll);

  int icon_cy = bar.origin.y + (bar.size.h / 2);
  if (s_footer_kind == 2) {
    graphics_context_set_fill_color(ctx, s_theme.text_secondary);
    graphics_fill_circle(ctx, GPoint(bar.origin.x + 14, icon_cy), 4);
  } else {
    GRect checkbox = GRect(bar.origin.x + 14, icon_cy - 5, 10, 10);
    graphics_context_set_stroke_color(ctx, s_theme.text_secondary);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_rect(ctx, checkbox);
  }

  graphics_context_set_text_color(ctx, s_theme.text_secondary);
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  GRect text_box = GRect(bar.origin.x + 28, bar.origin.y + 2,
                         bar.size.w - 32, bar.size.h - 4);
  graphics_draw_text(ctx, s_todo_text, font, text_box,
                     GTextOverflowModeWordWrap, GTextAlignmentLeft, false);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, s_theme.bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_tick_ring(ctx);
  draw_inner_frame(ctx);
  draw_habits(ctx);
  draw_todo_footer(ctx, bounds);
}

static void update_date(void) {
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  if (!tick_time) return;

  s_current_minute = tick_time->tm_min;
  s_current_hour_tick = ((tick_time->tm_hour % 12) * 5) % 60;

  static char weekday_buffer[8];
  static char day_buffer[12];
  strftime(weekday_buffer, sizeof(weekday_buffer), "%a", tick_time);
  strftime(day_buffer, sizeof(day_buffer), "%b %d", tick_time);
  text_layer_set_text(s_date_weekday_layer, weekday_buffer);
  text_layer_set_text(s_date_day_layer, day_buffer);
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

static void refresh_timer_callback(void *data) {
  request_hey_data();
  s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void schedule_refresh_timer(void) {
  if (s_refresh_timer) {
    app_timer_cancel(s_refresh_timer);
  }
  s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
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
  Tuple *footer_kind_tuple = dict_find(iterator, MESSAGE_KEY_FOOTER_KIND);
  Tuple *status_tuple = dict_find(iterator, MESSAGE_KEY_SYNC_STATUS);
  Tuple *theme_tuple = dict_find(iterator, MESSAGE_KEY_THEME_MODE);

  bool changed = false;

  if (theme_tuple) {
    uint8_t mode = theme_tuple->value->uint8 ? 1 : 0;
    if (mode != s_theme_mode) {
      apply_theme(mode);
      clear_habit_icon_cache();
      changed = true;
    }
  }

  if (status_tuple) {
    if (s_sync_status != status_tuple->value->uint8) {
      s_sync_status = status_tuple->value->uint8;
      changed = true;
    }
  }

  if (count_tuple) {
    uint8_t count = count_tuple->value->uint8;
    uint8_t mask = mask_tuple ? mask_tuple->value->uint8 : 0;
    const char *icons = icons_tuple ? icons_tuple->value->cstring : "";
    const char *colors = colors_tuple ? colors_tuple->value->cstring : "";

    if (habit_data_changed(count, mask, icons, colors) || count != s_habit_count) {
      clear_habit_icon_cache();
      s_habit_count = count;
      s_habit_done_mask = mask;
      for (int i = 0; i < MAX_HABITS; i++) {
        s_habit_icons[i][0] = '\0';
        s_habit_colors[i][0] = '\0';
      }
      for (int i = 0; i < count && i < MAX_HABITS; i++) {
        parse_pipe_field(s_habit_icons[i], ICON_SLUG_LEN, icons, i);
        parse_pipe_field(s_habit_colors[i], COLOR_SLUG_LEN, colors, i);
      }
      changed = true;
    }
  }

  if (todo_tuple) {
    const char *todo = todo_tuple->value->cstring;
    if (strcmp(todo, s_todo_text) != 0) {
      snprintf(s_todo_text, sizeof(s_todo_text), "%s", todo);
      changed = true;
    }
  }

  if (footer_kind_tuple) {
    uint8_t kind = footer_kind_tuple->value->uint8;
    if (kind != s_footer_kind) {
      s_footer_kind = kind;
      changed = true;
    }
  }

  if (changed) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_date();
  layer_mark_dirty(s_canvas_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  compute_layout(bounds);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  int weekday_y = s_layout.date_center_y - 22;
  s_date_weekday_layer = text_layer_create(GRect(0, weekday_y, bounds.size.w, 24));
  text_layer_set_background_color(s_date_weekday_layer, GColorClear);
  text_layer_set_text_color(s_date_weekday_layer, s_theme.ink);
  text_layer_set_font(s_date_weekday_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_date_weekday_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_weekday_layer));

  int day_y = s_layout.date_center_y - 2;
  s_date_day_layer = text_layer_create(GRect(0, day_y, bounds.size.w, 20));
  text_layer_set_background_color(s_date_day_layer, GColorClear);
  text_layer_set_text_color(s_date_day_layer, s_theme.text_secondary);
  text_layer_set_font(s_date_day_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_date_day_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_day_layer));

  update_date();
}

static void window_unload(Window *window) {
  if (s_refresh_timer) {
    app_timer_cancel(s_refresh_timer);
    s_refresh_timer = NULL;
  }
  clear_habit_icon_cache();
  layer_destroy(s_canvas_layer);
  text_layer_destroy(s_date_weekday_layer);
  text_layer_destroy(s_date_day_layer);
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
  app_message_open(2048, 256);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  request_hey_data();
  schedule_refresh_timer();
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
