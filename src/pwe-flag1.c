/*
 * main.c
 * Sets up a Window, BitmapLayer and blank GBitmap to be used as the display
 * container for the GBitmapSequence. It also counts the number of frames.
 *
 * Animation source:
 * http://bestanimations.com/Science/Physics/Physics2.html
 */

#include <pebble.h>

static Window *s_main_window;

static GBitmap *s_bitmap = NULL;
static BitmapLayer *s_bitmap_layer;
static BitmapLayer *s_bitmap_layer2;
static GBitmapSequence *s_sequence = NULL;
static Layer *s_canvas_layer = NULL;

static struct tm s_local_time;
static TextLayer *time_text_layer_time;
static TextLayer *time_text_layer_day;
static TextLayer *time_text_layer_date;
static TextLayer *time_text_layer_date2;
static char time_buffer[9];
static char day_buffer[9];
static char date_buffer[9];
static char date_buffer2[9];
static GFont s_custom_font;
static GFont s_custom_font2;
static GFont s_custom_font3;

static int s_tm_wday = 0;
static int s_tm_mon = 0;
static int s_tm_mday = 0;
static bool s_animation_enabled = true;

static void prv_update_day(int day) {
    switch (day) {
    case 0:
      snprintf(day_buffer, sizeof(day_buffer), "SUN");
      break;
    case 1:
      snprintf(day_buffer, sizeof(day_buffer), "MON");
      break;
    case 2:
      snprintf(day_buffer, sizeof(day_buffer), "TUE");
      break;
    case 3:
      snprintf(day_buffer, sizeof(day_buffer), "WED");
      break;
    case 4:
      snprintf(day_buffer, sizeof(day_buffer), "THU");
      break;
    case 5:
      snprintf(day_buffer, sizeof(day_buffer), "FRI");
      break;
    case 6:
      snprintf(day_buffer, sizeof(day_buffer), "SAT");
      break;
  }
  text_layer_set_text(time_text_layer_day, day_buffer);
}

static void prv_update_month(int month) {
  switch (month) {
    case 1:
      snprintf(date_buffer2, sizeof(date_buffer2), "JAN");
      break;
    case 2:
      snprintf(date_buffer2, sizeof(date_buffer2), "FEB");
      break;
    case 3:
      snprintf(date_buffer2, sizeof(date_buffer2), "MAR");
      break;
    case 4:
      snprintf(date_buffer2, sizeof(date_buffer2), "APR");
      break;
    case 5:
      snprintf(date_buffer2, sizeof(date_buffer2), "MAY");
      break;
    case 6:
      snprintf(date_buffer2, sizeof(date_buffer2), "JUN");
      break;
    case 7:
      snprintf(date_buffer2, sizeof(date_buffer2), "JUL");
      break;
    case 8:
      snprintf(date_buffer2, sizeof(date_buffer2), "AUG");
      break;
    case 9:
      snprintf(date_buffer2, sizeof(date_buffer2), "SEP");
      break;
    case 10:
      snprintf(date_buffer2, sizeof(date_buffer2), "OCT");
      break;
    case 11:
      snprintf(date_buffer2, sizeof(date_buffer2), "NOV");
      break;
    case 12:
      snprintf(date_buffer2, sizeof(date_buffer2), "DEC");
      break;
  }
  text_layer_set_text(time_text_layer_date2, date_buffer2);
}

static void prv_update_mday(int mday) {
  int day_l = mday / 10;
  int day_r = mday % 10;
  snprintf(date_buffer, sizeof(time_buffer), "%d%d", day_l, day_r);
  text_layer_set_text(time_text_layer_date, date_buffer);
}

static void timer_handler(void *context) {
  if (!s_animation_enabled) {
    return;
  }

  uint32_t next_delay;

  // Advance to the next APNG frame
  if(gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, &next_delay)) {
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer2));

    // Timer for that delay
    app_timer_register(next_delay, timer_handler, NULL);
  } else {
    // Start again
    gbitmap_sequence_restart(s_sequence);
  }
}

static void disable_animation(void *context) {
  s_animation_enabled = false;
}

static void update_time(struct tm *local_time) {
  memcpy(&s_local_time, local_time, sizeof(s_local_time));

  int hour = s_local_time.tm_hour;
  if (!clock_is_24h_style())
  {
    if (hour == 0) {
      hour = 12;
    } else if (hour > 12) {
      hour -= 12;
    }
  }

  int hour_l = hour / 10;
  int hour_r = hour % 10;
  int min_l = s_local_time.tm_min / 10;
  int min_r = s_local_time.tm_min % 10;
  snprintf(time_buffer, sizeof(time_buffer), "%d%d:%d%d",
           hour_l, hour_r, min_l, min_r);
  text_layer_set_text(time_text_layer_time, time_buffer);

  if ((s_local_time.tm_min % 15 == 0) && (!s_animation_enabled)) {
    s_animation_enabled = true;
    app_timer_register(1, timer_handler, NULL);
    app_timer_register(5000, disable_animation, NULL);
  }

  prv_update_day(s_local_time.tm_wday);

  prv_update_mday(s_local_time.tm_mday);

  prv_update_month(s_local_time.tm_mon + 1);
}

static void tap_enable_animation(AccelAxisType axis, int32_t direction) {
  if (!s_animation_enabled) {
    s_animation_enabled = true;
    app_timer_register(1, timer_handler, NULL);
    app_timer_register(5000, disable_animation, NULL);
  }
}

static void prv_update_time(struct tm *tick_time, TimeUnits units_changed) {
  // Make sure the time is displayed from the start
  time_t temp = time(NULL); 
  struct tm *local_time = localtime(&temp);
  update_time(local_time);
}

static void load_sequence() {
  // Free old data
  if(s_sequence) {
    gbitmap_sequence_destroy(s_sequence);
    s_sequence = NULL;
  }
  if(s_bitmap) {
    gbitmap_destroy(s_bitmap);
    s_bitmap = NULL;
  }

  // Create sequence
  s_sequence = gbitmap_sequence_create_with_resource(RESOURCE_ID_FLAG);

  // Create GBitmap
  s_bitmap = gbitmap_create_blank(gbitmap_sequence_get_bitmap_size(s_sequence), GBitmapFormat8Bit);

  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  bitmap_layer_set_bitmap(s_bitmap_layer2, s_bitmap);

  // Begin animation
  s_animation_enabled = true;
  app_timer_register(1, timer_handler, NULL);
  app_timer_register(5000, disable_animation, NULL);
}

static void draw_lines(GContext* ctx, GColor color, int16_t offset) {
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 3);

  graphics_draw_line(ctx, GPoint(0, 54 + offset - 1), GPoint(144, 54 + offset - 1));
  graphics_draw_line(ctx, GPoint(0, 168 - 54 - offset), GPoint(144, 168 - 54 - offset));

  graphics_draw_line(ctx, GPoint(72, -1 + offset), GPoint(144, -1 + offset));
  graphics_draw_line(ctx, GPoint(0, 168 - offset), GPoint(72, 168 - offset));
}
static void update_canvas_layer(struct Layer *layer, GContext* ctx) {
  draw_lines(ctx, GColorRed, 2);
  draw_lines(ctx, GColorWhite, 4);
  draw_lines(ctx, GColorBlue, 6);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_tm_wday = (s_tm_wday + 1) % 7;
  s_tm_mon = (s_tm_mon + 1) % 13;
  s_tm_mday = (s_tm_mday + 1) % 32;
  prv_update_day(s_tm_wday);
  prv_update_month(s_tm_mon);
  prv_update_mday(s_tm_mday);
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
}


static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, update_canvas_layer);
  layer_add_child(window_layer, s_canvas_layer);

  s_bitmap_layer = bitmap_layer_create(bounds);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
  bitmap_layer_set_alignment(s_bitmap_layer, GAlignTopLeft);

  s_bitmap_layer2 = bitmap_layer_create(bounds);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer2));
  bitmap_layer_set_alignment(s_bitmap_layer2, GAlignBottomRight);

  load_sequence();

  s_custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BLOCK_36));

  time_text_layer_time = text_layer_create(GRect(6, 54 + 4, 144, 54 + 8 + 48));
  text_layer_set_text_color(time_text_layer_time, GColorWhite);
  text_layer_set_background_color(time_text_layer_time, GColorClear);
  text_layer_set_text_alignment(time_text_layer_time, GTextAlignmentCenter);
  text_layer_set_font(time_text_layer_time, s_custom_font);
  layer_add_child(s_canvas_layer, text_layer_get_layer(time_text_layer_time));
  
  s_custom_font2 = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  time_text_layer_day = text_layer_create(GRect(-4, 168 - 54 + 4, 80, 48));
  text_layer_set_text_color(time_text_layer_day, GColorWhite);
  text_layer_set_background_color(time_text_layer_day, GColorClear);
  text_layer_set_text_alignment(time_text_layer_day, GTextAlignmentCenter);
  text_layer_set_font(time_text_layer_day, s_custom_font2);
  layer_add_child(s_canvas_layer, text_layer_get_layer(time_text_layer_day));

  time_text_layer_date2 = text_layer_create(GRect(72, 0, 72, 34));
  text_layer_set_text_color(time_text_layer_date2, GColorWhite);
  text_layer_set_background_color(time_text_layer_date2, GColorClear);
  text_layer_set_text_alignment(time_text_layer_date2, GTextAlignmentCenter);
  text_layer_set_overflow_mode(time_text_layer_date2, GTextOverflowModeWordWrap);
  text_layer_set_font(time_text_layer_date2, s_custom_font2);
  layer_add_child(s_canvas_layer, text_layer_get_layer(time_text_layer_date2));

  s_custom_font3 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BLOCK_24));

  time_text_layer_date = text_layer_create(GRect(72 + 6, 36 - 8, 72, 28));
  text_layer_set_text_color(time_text_layer_date, GColorWhite);
  text_layer_set_background_color(time_text_layer_date, GColorClear);
  text_layer_set_text_alignment(time_text_layer_date, GTextAlignmentCenter);
  text_layer_set_overflow_mode(time_text_layer_date, GTextOverflowModeWordWrap);
  text_layer_set_font(time_text_layer_date, s_custom_font3);
  layer_add_child(s_canvas_layer, text_layer_get_layer(time_text_layer_date));

  // Make sure the time is displayed from the start
  time_t temp = time(NULL); 
  struct tm *local_time = localtime(&temp);
  update_time(local_time);

  tick_timer_service_subscribe(MINUTE_UNIT, prv_update_time);

  accel_tap_service_subscribe(tap_enable_animation);
}


static void main_window_unload(Window *window) {
  bitmap_layer_destroy(s_bitmap_layer);
  accel_tap_service_unsubscribe();
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
#ifdef PBL_SDK_2
  window_set_fullscreen(s_main_window, true);
#endif
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });

  window_set_click_config_provider(s_main_window, click_config_provider);

  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
