#include <pebble.h>

#define NUM_SIZE 36
#define NUM_M_SIZE 48

#define ATLAS_NUM 13
#define ATLAS_HR 11
#define ATLAS_MIN 12

static Window* window;

static GBitmap* font_bitmap;
static GBitmap* font_m_bitmap;

static BitmapLayer* hr_digits[3];
static BitmapLayer* hr_unit;
static BitmapLayer* min_digits[3];
static BitmapLayer* min_unit;

static int window_width, window_height;

// -----------------------------------------------------------------------------

static bool is_rand_seed_set = false;

static void set_random_seed()
{
  if (false == is_rand_seed_set)
  {
    srand((unsigned int)time(NULL));
    is_rand_seed_set = true;
  }
}

int range_random(int min, int max)
{
  set_random_seed();

  if (min > max)
  {
    int tmp = min;
    min = max;
    max = tmp;
  }
  
  return min + rand() % (max - min + 1);
}

// -----------------------------------------------------------------------------

int current_hr = -1;
int current_min = -1;

static int format_hr(int hr)
{
  if (!clock_is_24h_style())
  {
    if (hr == 0) hr += 12;
    if (hr > 12) hr -= 12;
  }

  return hr;
}

static int calculate_digits(int value, int* result_digits)
{
  int digit_num = 0;

  int in_ones = value % 10;
  int in_tens = value / 10;

  if (in_tens > 1 && in_ones > 0)
  {
    digit_num = 3;
    result_digits[0] = in_tens;
    result_digits[1] = 10;
    result_digits[2] = in_ones;
  }
  else if (value > 10)
  {
    digit_num = 2;
    result_digits[0] = in_tens >= 2 ? in_tens : 10;
    result_digits[1] = in_tens >= 2 ? 10 : in_ones;
  }
  else
  {
    digit_num = 1;
    result_digits[0] = in_tens > 0 ? 10 : in_ones;
  }

  return digit_num;
}

static void apply_bitmap_atlas(BitmapLayer* bitmap_layer, GBitmap* bitmap, int unit_width, int unit_num, int idx)
{
  bitmap_layer_set_bitmap(bitmap_layer, bitmap);
  layer_set_bounds(bitmap_layer_get_layer(bitmap_layer), GRect(-idx * unit_width / 2, 0, unit_width * unit_num, unit_width)); // why need to divide 2?
}

static void refresh_time()
{
  int hr_digit_idxs[3];
  int hr_digit_num;
  int min_digit_idxs[3];
  int min_digit_num;

  Layer* layer;

  hr_digit_num = calculate_digits(current_hr, hr_digit_idxs);

  if (current_min > 0)
    min_digit_num = calculate_digits(current_min, min_digit_idxs);
  else
    min_digit_num = 0; // if 0 min, hide it

  GBitmap* hr_font_bitmap = font_bitmap;
  int hr_size = NUM_SIZE;
  if (hr_digit_num < 3)
  {
    hr_size = NUM_M_SIZE;
    hr_font_bitmap = font_m_bitmap;
  }
  int hr_left = (window_width - (hr_digit_num + 1) * hr_size) / 2;

  GBitmap* min_font_bitmap = font_bitmap;
  int min_size = NUM_SIZE;
  if (min_digit_num == 0)
  {
    min_size = 0;
  }
  else if (min_digit_num < 3)
  {
    min_size = NUM_M_SIZE;
    min_font_bitmap = font_m_bitmap;
  }
  int min_left = (window_width - (min_digit_num + 1) * min_size) / 2;

  int top = (window_height - hr_size - min_size) / 2;

  for (int i = 0; i < 3; ++i)
  {
    // hr

    layer = bitmap_layer_get_layer(hr_digits[i]);

    if (i < hr_digit_num)
    {
      apply_bitmap_atlas(hr_digits[i], hr_font_bitmap, hr_size, ATLAS_NUM, hr_digit_idxs[i]);
      layer_set_frame(layer, GRect(hr_left + hr_size * i, top, hr_size, hr_size));
      layer_set_hidden(layer, false);
    }
    else
    {
      layer_set_hidden(layer, true);
    }

    // min

    layer = bitmap_layer_get_layer(min_digits[i]);

    if (i < min_digit_num)
    {
      apply_bitmap_atlas(min_digits[i], min_font_bitmap, min_size, ATLAS_NUM, min_digit_idxs[i]);
      layer_set_frame(layer, GRect(min_left + min_size * i, top + hr_size, min_size, min_size));
      layer_set_hidden(layer, false);
    }
    else
    {
      layer_set_hidden(layer, true);
    }
  }

  // hr unit

  layer = bitmap_layer_get_layer(hr_unit);
  apply_bitmap_atlas(hr_unit, hr_font_bitmap, hr_size, ATLAS_NUM, ATLAS_HR);
  layer_set_frame(layer, GRect(hr_left + hr_size * hr_digit_num, top, hr_size, hr_size));

  // min unit

  layer = bitmap_layer_get_layer(min_unit);
  if (min_digit_num > 0)
  {
    apply_bitmap_atlas(min_unit, min_font_bitmap, min_size, ATLAS_NUM, ATLAS_MIN);
    layer_set_frame(layer, GRect(min_left + min_size * min_digit_num, top + hr_size, min_size, min_size));
    layer_set_hidden(layer, false);
  }
  else
  {
    layer_set_hidden(layer, true);
  }
}

// -----------------------------------------------------------------------------

#define STAR_TRANSITION_PERIOD 1.6f
#define STAR_HALF_SIZE 5.f
#define SCALE_SPEED 10.f
#define MAX_SCALE 6.f
#define SPAWN_PERIOD 0.02f

static Layer* star_layer;
static GPath* star_path;

static GPathInfo base_star_path_info =
{
  // This is the amount of points
  8,
  // A path can be concave, but it should not twist on itself
  // The points should be defined in clockwise order due to the rendering
  // implementation. Counter-clockwise will work in older firmwares, but
  // it is not officially supported
  (GPoint [])
  {
    { 0, -STAR_HALF_SIZE },
    { 1,              -1 },
    { STAR_HALF_SIZE,  0 },
    { 1,               1 },
    { 0,  STAR_HALF_SIZE },
    { -1,              1 },
    { -STAR_HALF_SIZE, 0 },
    { -1,             -1 }
  }
};

static GPathInfo curr_star_path_info;

static void ApplyPathBaseToCurrent(float scale)
{
  for (unsigned int i = 0; i < base_star_path_info.num_points; ++i)
  {
    curr_star_path_info.points[i].x = base_star_path_info.points[i].x * scale;
    curr_star_path_info.points[i].y = base_star_path_info.points[i].y * scale;
  }
}

// -----------------------------------------------------------------------------

#define START_POOL_SIZE 16

typedef struct
{
  float scale;
  GPoint pos;
  bool in_use;
} StarInfo;

static StarInfo star_pool[START_POOL_SIZE];

static void spawn_star()
{
  StarInfo* star = NULL;
  for (int i = 0; i < START_POOL_SIZE; ++i)
  {
    if (!star_pool[i].in_use)
    {
      star = &star_pool[i];
      break;
    }
  }

  if (star)
  {
    star->scale = 1.f;

    float border = MAX_SCALE * STAR_HALF_SIZE;
    star->pos.x = range_random(border, window_width - border);
    star->pos.y = range_random(border, window_height - border);

    star->in_use = true;
  }
  else
  {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "No usable star in pool");
  }
}

// -----------------------------------------------------------------------------

static bool need_refresh_time = false;

static AnimationImplementation anim_impl;
static Animation* anim;

static float prev_ratio, max_spawn_ratio, spawn_timer;
static bool time_refreshed;

static void anim_setup(struct Animation* animation)
{
  prev_ratio = 0.f;
  max_spawn_ratio = (STAR_TRANSITION_PERIOD - ((MAX_SCALE - 1.f) / SCALE_SPEED)) / STAR_TRANSITION_PERIOD;
  spawn_timer = 0.f;
  time_refreshed = false;
}

static void anim_update(struct Animation* animation, const uint32_t time_normalized)
{
  float ratio = (float)time_normalized / ANIMATION_NORMALIZED_MAX;
  float delta_time = (ratio - prev_ratio) * STAR_TRANSITION_PERIOD;
  prev_ratio = ratio;

  if (need_refresh_time && !time_refreshed && ratio >= 0.25f) // refresh time in transition
  {
    refresh_time();

    time_refreshed = true;
    need_refresh_time = false;
  }

  if (ratio < max_spawn_ratio)
  {
    spawn_timer -= delta_time;
    if (spawn_timer <= 0.f)
    {
      spawn_star();
      spawn_timer = SPAWN_PERIOD;
    }
  }

  for (int i = 0; i < START_POOL_SIZE; ++i)
  {
    if (star_pool[i].in_use)
    {
      star_pool[i].scale += delta_time * SCALE_SPEED;
      if (star_pool[i].scale > MAX_SCALE)
        star_pool[i].in_use = false;
    }
  }

  layer_mark_dirty(star_layer);
}

static void anim_teardown(struct Animation* animation)
// static void anim_stopped(struct Animation* animation, bool finished, void *context)
{
  for (int i = 0; i < START_POOL_SIZE; ++i)
    star_pool[i].in_use = false;

  layer_mark_dirty(star_layer);
}

// -----------------------------------------------------------------------------

static void star_layer_update_callback(Layer *me, GContext *ctx)
{
  for (int i = 0; i < START_POOL_SIZE; ++i)
  {
    if (star_pool[i].in_use)
    {
      ApplyPathBaseToCurrent(star_pool[i].scale);

      gpath_move_to(star_path, star_pool[i].pos);

      graphics_context_set_fill_color(ctx, GColorWhite);
      gpath_draw_filled(ctx, star_path);
    }
  }
}

static void init_star_transition(Layer* window_layer, GRect* bounds)
{
  for (int i = 0; i < START_POOL_SIZE; ++i)
    star_pool[i].in_use = false;

  star_layer = layer_create(*bounds);
  layer_set_update_proc(star_layer, star_layer_update_callback);
  layer_add_child(window_layer, star_layer);

  curr_star_path_info.num_points = base_star_path_info.num_points;
  curr_star_path_info.points = malloc(sizeof(GPoint) * curr_star_path_info.num_points);

  star_path = gpath_create(&curr_star_path_info);

  anim_impl.setup = anim_setup;
  anim_impl.update = anim_update;
  anim_impl.teardown = anim_teardown;
  anim = animation_create();
  animation_set_delay(anim, 0);
  animation_set_duration(anim, (int)(STAR_TRANSITION_PERIOD * 1000));
  animation_set_implementation(anim, &anim_impl);
}

static void deinit_start_transition()
{
  animation_destroy(anim);
  layer_destroy(star_layer);
  gpath_destroy(star_path);
  free(curr_star_path_info.points);
}

// -----------------------------------------------------------------------------

static void handle_min_tick(struct tm* time, TimeUnits units_changed)
{
  int now_hr = format_hr(time->tm_hour);
  int now_min = time->tm_min;

  if (current_hr != now_hr || current_min != now_min)
  {
    current_hr = now_hr;
    current_min = now_min;

    need_refresh_time = true;
  }

  animation_schedule(anim);
}

// -----------------------------------------------------------------------------

static void window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_width = bounds.size.w;
  window_height = bounds.size.h;

  //

  font_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FONT);
  font_m_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FONT_M);

  //

  GRect rect = GRect(0, 0, NUM_SIZE, NUM_SIZE);

  Layer* layer;
  for (int i = 0; i < 3; ++i)
  {
    hr_digits[i] = bitmap_layer_create(rect);
    layer = bitmap_layer_get_layer(hr_digits[i]);
    layer_add_child(window_layer, layer);
    layer_set_hidden(layer, true);

    min_digits[i] = bitmap_layer_create(rect);
    layer = bitmap_layer_get_layer(min_digits[i]);
    layer_add_child(window_layer, layer);
    layer_set_hidden(layer, true);
  }

  hr_unit = bitmap_layer_create(rect);
  layer_add_child(window_layer, bitmap_layer_get_layer(hr_unit));

  min_unit = bitmap_layer_create(rect);
  layer_add_child(window_layer, bitmap_layer_get_layer(min_unit));

  //

  init_star_transition(window_layer, &bounds);

  time_t timestamp = time(NULL);
  struct tm* time = localtime(&timestamp);

  current_hr = format_hr(time->tm_hour);
  current_min = time->tm_min;
  refresh_time();

  tick_timer_service_subscribe(MINUTE_UNIT, handle_min_tick);
}

static void window_unload(Window *window)
{
  deinit_start_transition();

  bitmap_layer_destroy(hr_unit);
  bitmap_layer_destroy(min_unit);

  for (int i = 0; i < 3; ++i)
  {
    bitmap_layer_destroy(hr_digits[i]);
    bitmap_layer_destroy(min_digits[i]);
  }

  gbitmap_destroy(font_m_bitmap);
  gbitmap_destroy(font_bitmap);
}

static void init(void)
{
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void)
{
  window_destroy(window);
}

int main(void)
{
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
