#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;
static GBitmap *s_dragon_bitmap = NULL;

static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    if (s_dragon_bitmap) {
        GRect bitmap_bounds = gbitmap_get_bounds(s_dragon_bitmap);
        int x = (bounds.size.w - bitmap_bounds.size.w) / 2;
        int y = (bounds.size.h - bitmap_bounds.size.h) / 2;
        graphics_draw_bitmap_in_rect(ctx, s_dragon_bitmap, GRect(x, y, bitmap_bounds.size.w, bitmap_bounds.size.h));
    }
    
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    
    if (tick_time) {
        int hours = tick_time->tm_hour % 12;
        if (hours == 0) hours = 12;
        int minutes = tick_time->tm_min;
        
        static char time_buffer[8];
        snprintf(time_buffer, sizeof(time_buffer), "%d:%02d", hours, minutes);
        
        graphics_context_set_text_color(ctx, GColorDarkGreen);
        
        GFont font = fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
        GRect time_frame = GRect(0, 85, bounds.size.w, 50);
        graphics_draw_text(ctx, time_buffer, font, time_frame, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        
        char date_buffer[24];
        strftime(date_buffer, sizeof(date_buffer), "%a, %b %d", tick_time);
        
        GFont date_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
        GRect date_frame = GRect(0, 130, bounds.size.w, 30);
        graphics_draw_text(ctx, date_buffer, date_font, date_frame, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_dragon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DRAGON_SIGN_IMAGE);

    s_canvas_layer = layer_create(bounds);
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);
    
    layer_mark_dirty(s_canvas_layer);
}

static void main_window_unload(Window *window) {
    if (s_dragon_bitmap) {
        gbitmap_destroy(s_dragon_bitmap);
        s_dragon_bitmap = NULL;
    }

    if (s_canvas_layer) {
        layer_destroy(s_canvas_layer);
        s_canvas_layer = NULL;
    }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    if (s_canvas_layer) {
        layer_mark_dirty(s_canvas_layer);
    }
}

static void init(void) {
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    window_stack_push(s_main_window, true);

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
    tick_timer_service_unsubscribe();

    if (s_main_window) {
        window_destroy(s_main_window);
        s_main_window = NULL;
    }
}

int main(void) {
    init();
    app_event_loop();
    deinit();
    return 0;
}
