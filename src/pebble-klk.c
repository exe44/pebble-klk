#include <pebble.h>
#include <ctype.h>
#include "gbitmap_color_palette_manipulator.h"

//#define DEBUG

#define NUM_S_SIZE  24
#define NUM_M_SIZE  36
#define NUM_L_SIZE  48
#define NUM_SPAN_SIZE 2

#ifdef PBL_PLATFORM_CHALK
  #define NUM_OFFSET  6
  #define NUM_OFFSET_TWO_CHAR   8
  #define NUM_OFFSET_DATE_MONTH 4
#else
  #define NUM_OFFSET  12
  #define NUM_OFFSET_TWO_CHAR   0
  #define NUM_OFFSET_DATE_MONTH 0
#endif

#ifdef PBL_PLATFORM_APLITE
  #define ATLAS_NUM_X       16
  #define ATLAS_NUM_Y_ML    1
  #define ATLAS_NUM_Y_S     1
#else
  #define ATLAS_NUM_X       11
  #define ATLAS_NUM_Y_ML    3
  #define ATLAS_NUM_Y_S     5
#endif

#define CHAR_MAX_LENGTH   6
#define NORMAL_NUM_ROW    0
#define FORMAL_NUM_ROW    1

#ifdef DEBUG
static int debug_hour =   1;
static int debug_min =    11;
static int debug_date =   31;
static int debug_month =  11;
#endif

// -----------------------------------------------------------------------------
// atlas setting
// -----------------------------------------------------------------------------

struct CharAtlas {
  int num;                          // character num
  int atlas[CHAR_MAX_LENGTH][2];    // atlas array({{x,y}, {x,y}, ..})
};

// 2 時分秒年月日午前後AMPM
#ifdef PBL_PLATFORM_APLITE
  static struct CharAtlas s_atlas_hour_suffix =  { 1, { { 11, 0 } } };
  static struct CharAtlas s_atlas_min_suffix =   { 1, { { 12, 0 } } };
  static struct CharAtlas s_atlas_sec_suffix =   { 1, { { 13, 0 } } };
  static struct CharAtlas s_atlas_year_suffix =  { 1, { { 11, 0 } } };
  static struct CharAtlas s_atlas_month_suffix = { 1, { { 12, 0 } } };
  static struct CharAtlas s_atlas_date_suffix =  { 1, { { 13, 0 } } };
  static struct CharAtlas s_atlas_am_suffix =    { 1, { { 14, 0 } } };
  static struct CharAtlas s_atlas_pm_suffix =    { 1, { { 15, 0 } } };
#else
  static struct CharAtlas s_atlas_hour_suffix =  { 1, { { 0, 2 } } };
  static struct CharAtlas s_atlas_min_suffix =   { 1, { { 1, 2 } } };
  static struct CharAtlas s_atlas_sec_suffix =   { 1, { { 2, 2 } } };
  static struct CharAtlas s_atlas_year_suffix =  { 1, { { 3, 2 } } };
  static struct CharAtlas s_atlas_month_suffix = { 1, { { 4, 2 } } };
  static struct CharAtlas s_atlas_date_suffix =  { 1, { { 5, 2 } } };
  static struct CharAtlas s_atlas_am_suffix =    { 1, { { 9, 2 } } };
  static struct CharAtlas s_atlas_pm_suffix =    { 1, { { 10, 2 } } };
#endif

// 3 睦如弥生卯皐水無第
// 4 文葉長神在霜師走月
static struct CharAtlas s_atlas_lunar[12] = {
  { 2, { { 0,3 },{ 8,4 } } },           // 睦月
  { 2, { { 1,3 },{ 8,4 } } },           // 如月
  { 2, { { 2,3 },{ 3,3 } } },           // 弥生
  { 2, { { 4,3 },{ 8,4 } } },           // 卯月
  { 2, { { 5,3 },{ 8,4 } } },           // 皐月
  { 3, { { 6,3 },{ 7,3 },{ 8,4 } } },   // 水無月
  { 2, { { 0,4 },{ 8,4 } } },           // 文月
  { 2, { { 1,4 },{ 8,4 } } },           // 葉月
  { 2, { { 2,4 },{ 8,4 } } },           // 長月
  { 3, { { 3,4 },{ 7,3 },{ 8,4 } } },   // 神無月
  { 2, { { 5,4 },{ 8,4 } } },           // 霜月
  { 2, { { 6,4 },{ 7,4 } } }            // 師走
};
#ifdef PBL_PLATFORM_APLITE
  static struct CharAtlas s_atlas_prefix = { 1, { { 14,0 } } };
#else
  static struct CharAtlas s_atlas_prefix = { 1, { { 8,3 } } };
#endif

// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

enum PersistKey
{
  PERSIST_CONFIG = 0
};

enum MessageKey
{
  MSG_CONFIG_BG_COLOR = 0,
  MSG_CONFIG_STAR_COLOR,
  MSG_CONFIG_TIME_COLOR,
  MSG_CONFIG_DATE_COLOR,
  MSG_CONFIG_MONTH_COLOR,
  MSG_CONFIG_IS_ENABLE_DATE,
  MSG_CONFIG_IS_ENABLE_MONTH,
  MSG_CONFIG_IS_USE_AMPM,
  MSG_CONFIG_IS_USE_LUNAR,
  MSG_CONFIG_IS_USE_PREFIX,
  MSG_CONFIG_IS_USE_FORMAL,
  MSG_CONFIG_DATE_POSITION_TYPE
};

enum DatePositionType
{
  DATE_POSITION_TOP = 0,
  DATE_POSITION_BOTTOM,
};

// -----------------------------------------------------------------------------

struct ConfigData
{
  GColor bg_color;
  GColor star_color;
  GColor time_color;
  GColor date_color;
  GColor month_color;
  bool is_enable_date;
  bool is_enable_month;
  bool is_use_ampm;
  bool is_use_lunar;
  bool is_use_prefix;
  bool is_use_formal;
  enum DatePositionType date_position_type;
};

static struct ConfigData config_data;

// -----------------------------------------------------------------------------

static void init_config()
{
  config_data.bg_color = GColorBlack;
  config_data.star_color = GColorWhite;
  config_data.time_color = GColorWhite;
  config_data.date_color = GColorWhite;
  config_data.month_color = GColorWhite;

  config_data.is_enable_date = true;
  config_data.is_enable_month = false;
  config_data.date_position_type = DATE_POSITION_TOP;

#ifdef PBL_PLATFORM_APLITE
  config_data.is_use_ampm = false;
  config_data.is_use_lunar = false;
  config_data.is_use_prefix = true;
  config_data.is_use_formal = false;
#else
  config_data.is_use_ampm = true;
  config_data.is_use_lunar = true;
  config_data.is_use_prefix = true;
  config_data.is_use_formal = false;
#endif

  if (persist_exists(PERSIST_CONFIG))
  {
    int config_size = sizeof(config_data);
    int persist_size = persist_get_size(PERSIST_CONFIG);
    if (persist_size != config_size)
    {
      APP_LOG(APP_LOG_LEVEL_WARNING, "config data size not match! need (%d), load (%d). discard config!", config_size, persist_size);
      persist_delete(PERSIST_CONFIG);
      return;
    }

    persist_read_data(PERSIST_CONFIG, &config_data, persist_size);
    APP_LOG(APP_LOG_LEVEL_INFO, "config loaded.");
  }
  else
  {
    APP_LOG(APP_LOG_LEVEL_INFO, "config not exist, inited.");
  }
}

static void save_config()
{
  int config_size = sizeof(config_data);
  if (config_size >= PERSIST_DATA_MAX_LENGTH)
  {
    APP_LOG(APP_LOG_LEVEL_ERROR, "config data size exceed maximum size! need (%d), max (%d). ignore save!", config_size, PERSIST_DATA_MAX_LENGTH);
    return;
  }

  persist_write_data(PERSIST_CONFIG, &config_data, config_size);
  APP_LOG(APP_LOG_LEVEL_INFO, "config saved.");
}

// -----------------------------------------------------------------------------
// static variable
// -----------------------------------------------------------------------------

static Window* window;

static GBitmap* font_s_bitmap_date = NULL;
static GBitmap* font_s_bitmap_month = NULL;
static GBitmap* font_m_bitmap = NULL;
static GBitmap* font_l_bitmap = NULL;

static BitmapLayer* hour_layers[CHAR_MAX_LENGTH];
static BitmapLayer* min_layers[CHAR_MAX_LENGTH];
static BitmapLayer* month_layers[CHAR_MAX_LENGTH];
static BitmapLayer* date_layers[CHAR_MAX_LENGTH];

static int window_width, window_height;

// -----------------------------------------------------------------------------
// random
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
// time format
// -----------------------------------------------------------------------------

int current_hr = -1;
int current_min = -1;
int current_date = -1;
int current_month = -1;

static int format_hr(int hr, int min, bool is_24h)
{
  // 00:00 = 00:00 a.m., 00:01 = 00:01 a.m.
  // 12:00 = 12:00 a.m., 12:01 = 00:01 p.m.
  int result = hr;
  if (!is_24h)
  {
    if (hr >= 12) result -= 12;
    if (hr == 0  && min == 0) result = 0;
    if (hr == 12 && min == 0) result = 12;
  }

  return result;
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

static int append_atlas(struct CharAtlas* dest_atlas, struct CharAtlas* src_atlas, int start_index)
{
  if (dest_atlas == NULL) return start_index;
  if (src_atlas == NULL) return start_index;

  int index = start_index;
  for (int i = 0; i < src_atlas->num; i++)
  {
    if (index >= CHAR_MAX_LENGTH) break;
    dest_atlas->atlas[index][0] = src_atlas->atlas[i][0];
    dest_atlas->atlas[index][1] = src_atlas->atlas[i][1];
    dest_atlas->num = index+1;
    index++;
  }

  // return next index
  return index;
}

static bool get_num_atlas(struct CharAtlas* atlas, int value, bool is_use_formal)
{
  if (atlas == NULL) return false;

  int digit_idxs[CHAR_MAX_LENGTH];
  int row = (is_use_formal) ? FORMAL_NUM_ROW : NORMAL_NUM_ROW;

  atlas->num = calculate_digits(value, digit_idxs);
  for (int i=0; i<atlas->num; i++) {
    atlas->atlas[i][0] = digit_idxs[i];
    atlas->atlas[i][1] = row;
  }

  return true;
}

static bool get_lunar_atlas(struct CharAtlas* atlas, int value)
{
  if (atlas == NULL) return false;
  if (value < 0 || value >= 12) return false;

  atlas->num = s_atlas_lunar[value].num;
  for (int i=0; i<atlas->num; i++) {
    atlas->atlas[i][0] = s_atlas_lunar[value].atlas[i][0];
    atlas->atlas[i][1] = s_atlas_lunar[value].atlas[i][1];
  }

  return true;
}

static bool is_am_time(int hour, int min) {
  // 00:00 = 00:00 a.m., 00:01 = 00:01 a.m.
  // 12:00 = 12:00 a.m., 12:01 = 00:01 p.m.
  bool is_am = (hour < 12);
  if (hour == 0  && min == 0) is_am = true;
  if (hour == 12 && min == 0) is_am = true;

  return is_am;
}

static void get_hour_atlas(struct CharAtlas* atlas, int hour, int min, bool is_24h, bool is_use_ampm, bool is_use_formal)
{
  if (atlas == NULL) return;

  int index = 0;
  struct CharAtlas atlas_num;

  if (!is_24h && is_use_ampm) {
    index = (is_am_time(hour, min)) ? append_atlas(atlas, &s_atlas_am_suffix, index) : append_atlas(atlas, &s_atlas_pm_suffix, index);
  }

  hour = format_hr(hour, min, is_24h);
  get_num_atlas(&atlas_num, hour, is_use_formal);
  index = append_atlas(atlas, &atlas_num, index);

  index = append_atlas(atlas, &s_atlas_hour_suffix, index);
}

static void get_min_atlas(struct CharAtlas* atlas, int min, bool is_use_formal)
{
  if (atlas == NULL) return;

  int index = 0;
  struct CharAtlas atlas_num;

  get_num_atlas(&atlas_num, min, is_use_formal);
  index = append_atlas(atlas, &atlas_num, index);

  index = append_atlas(atlas, &s_atlas_min_suffix, index);
}

static void get_date_atlas(struct CharAtlas* atlas, int date, bool is_use_prefix, bool is_use_formal)
{
  if (atlas == NULL) return;

  int index = 0;
  struct CharAtlas atlas_num;

  if (is_use_prefix) {
    index = append_atlas(atlas, &s_atlas_prefix, index);
  }

  get_num_atlas(&atlas_num, date, is_use_formal);
  index = append_atlas(atlas, &atlas_num, index);

  index = append_atlas(atlas, &s_atlas_date_suffix, index);
}

static void get_month_atlas(struct CharAtlas* atlas, int month, bool is_use_lunar, bool is_use_prefix, bool is_use_formal)
{
  if (atlas == NULL) return;

  int index = 0;
  struct CharAtlas atlas_num;

  if (is_use_prefix) {
    index = append_atlas(atlas, &s_atlas_prefix, index);
  }

  if (is_use_lunar)
  {
    get_lunar_atlas(&atlas_num, month);
    index = append_atlas(atlas, &atlas_num, index);
  }
  else {
    get_num_atlas(&atlas_num, month, is_use_formal);
    index = append_atlas(atlas, &atlas_num, index);

    index = append_atlas(atlas, &s_atlas_month_suffix, index);
  }
}

// -----------------------------------------------------------------------------
// time rendering
// -----------------------------------------------------------------------------

static void apply_bitmap_atlas(BitmapLayer* bitmap_layer, GBitmap* bitmap, int unit, int idx_x, int idx_y, int num_x, int num_y)
{
  bitmap_layer_set_bitmap(bitmap_layer, bitmap);

  int bound_origin_x = -idx_x * unit;
  int bound_origin_y = -idx_y * unit;
#ifdef PBL_PLATFORM_APLITE
  // bound_origin_x /= 2; // why need to divide 2?
  // bound_origin_y /= 2; // why need to divide 2?
#endif

  layer_set_bounds(bitmap_layer_get_layer(bitmap_layer), GRect(bound_origin_x, bound_origin_y, num_x * unit, num_y * unit));
}

static void render_atlas(BitmapLayer** bitmap_layers, GBitmap* bitmap, struct CharAtlas* atlas, int size, int top, int left, int num_x, int num_y)
{
  if (bitmap_layers == NULL) return;
  if (bitmap == NULL) return;
  if (atlas == NULL) return;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "render atlas: %d", atlas->num);
  for (int i = 0; i < CHAR_MAX_LENGTH; ++i)
  {
    Layer* layer = bitmap_layer_get_layer(bitmap_layers[i]);

    if (i < atlas->num)
    {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "render: %d (%d,%d)", i, atlas->atlas[i][0], atlas->atlas[i][1]);

      apply_bitmap_atlas(bitmap_layers[i], bitmap, size, atlas->atlas[i][0], atlas->atlas[i][1], num_x, num_y);
      layer_set_frame(layer, GRect(left + size * i, top, size, size));
      layer_set_hidden(layer, false);
    }
    else
    {
      layer_set_hidden(layer, true);
    }
  }
}

static void refresh_time()
{
  struct CharAtlas atlas_hour;
  struct CharAtlas atlas_min;
  struct CharAtlas atlas_date;
  struct CharAtlas atlas_month;

  // make atlas

  get_hour_atlas(&atlas_hour, current_hr, current_min, clock_is_24h_style(), config_data.is_use_ampm, config_data.is_use_formal);
  if (current_min > 0)
    get_min_atlas(&atlas_min, current_min, config_data.is_use_formal);
  else
    atlas_min.num = 0;          // if 0 min, hide it

  int hr_size = NUM_M_SIZE;
  GBitmap* hr_font_bitmap = font_m_bitmap;
  if (atlas_hour.num <= 3)
  {
    hr_size = NUM_L_SIZE;
    hr_font_bitmap = font_l_bitmap;
  }
  int hr_left = (window_width - (atlas_hour.num * hr_size)) / 2;

  int min_size = NUM_M_SIZE;
  GBitmap* min_font_bitmap = font_m_bitmap;
  if (atlas_min.num <= 3)
  {
    min_size = NUM_L_SIZE;
    min_font_bitmap = font_l_bitmap;
  }
  int min_left = (window_width - (atlas_min.num  * min_size)) / 2;

  int min_size_y = (atlas_min.num == 0) ? 0 : min_size;       // tm_min == 0
  int hr_top = (window_height - (hr_size + min_size_y)) / 2;
  int min_top = hr_top + hr_size;

  int date_size = NUM_S_SIZE;
  int date_top = 0;
  int date_left = 0;
  GBitmap* date_font_bitmap = font_s_bitmap_date;
  if (config_data.is_enable_date)
  {
    bool is_use_prefix = (config_data.is_use_prefix && (!config_data.is_enable_month || config_data.date_position_type == DATE_POSITION_TOP));
    get_date_atlas(&atlas_date, current_date, is_use_prefix, config_data.is_use_formal);

    date_top = (config_data.date_position_type == DATE_POSITION_TOP) ? hr_top - (date_size + NUM_SPAN_SIZE) : min_top + (min_size_y + NUM_SPAN_SIZE);
    date_left = (window_width - (atlas_date.num * date_size)) / 2;
  }
  else {
    for (int i = 0; i < CHAR_MAX_LENGTH; ++i)
    {
      layer_set_hidden(bitmap_layer_get_layer(date_layers[i]), true);
    }
  }

  int month_size = NUM_S_SIZE;
  int month_top = 0;
  int month_left = 0;
  GBitmap* month_font_bitmap = font_s_bitmap_month;
  if (config_data.is_enable_month)
  {
    bool is_use_prefix = (config_data.is_use_prefix && (!config_data.is_enable_date || config_data.date_position_type == DATE_POSITION_BOTTOM));
    get_month_atlas(&atlas_month, current_month, config_data.is_use_lunar, is_use_prefix, config_data.is_use_formal);

    month_top = (config_data.date_position_type != DATE_POSITION_TOP) ? hr_top - (month_size + NUM_SPAN_SIZE) : min_top + (min_size_y + NUM_SPAN_SIZE);
    month_left = (window_width - (atlas_month.num * month_size)) / 2;
  }
  else {
    for (int i = 0; i < CHAR_MAX_LENGTH; ++i)
    {
      layer_set_hidden(bitmap_layer_get_layer(month_layers[i]), true);
    }
  }

  // calc offset

  int offset = 0;
  if (config_data.is_enable_date && !config_data.is_enable_month)
  {
    if (config_data.date_position_type == DATE_POSITION_TOP)
    {
      offset = NUM_OFFSET + ((atlas_min.num <= 2) ? NUM_OFFSET_TWO_CHAR : 0);
    }
    else
    {
      offset = -(NUM_OFFSET + ((atlas_hour.num <= 2) ? NUM_OFFSET_TWO_CHAR : 0));
    }
  }
  if (!config_data.is_enable_date && config_data.is_enable_month)
  {
    if (config_data.date_position_type == DATE_POSITION_TOP)
    {
      offset = -(NUM_OFFSET + ((atlas_hour.num <= 2) ? NUM_OFFSET_TWO_CHAR : 0));
    }
    else
    {
      offset = NUM_OFFSET + ((atlas_min.num <= 2) ? NUM_OFFSET_TWO_CHAR : 0);
    }
  }
  if (config_data.is_enable_date && config_data.is_enable_month)
  {
    if (config_data.date_position_type == DATE_POSITION_TOP)
    {
      if      (atlas_date.num < atlas_month.num) offset = -NUM_OFFSET_DATE_MONTH;
      else if (atlas_date.num > atlas_month.num) offset =  NUM_OFFSET_DATE_MONTH;
    }
    else
    {
      if      (atlas_date.num < atlas_month.num) offset =  NUM_OFFSET_DATE_MONTH;
      else if (atlas_date.num > atlas_month.num) offset = -NUM_OFFSET_DATE_MONTH;
    }
  }

  // render

  APP_LOG(APP_LOG_LEVEL_DEBUG, "window: (%d, %d)", window_width, window_height);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "hour: %d, size=%d, top=%d, left=%d", current_hr, hr_size, hr_top, hr_left);
  render_atlas(hour_layers, hr_font_bitmap, &atlas_hour, hr_size, hr_top + offset, hr_left, ATLAS_NUM_X, ATLAS_NUM_Y_ML);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "min: %d, size=%d, top=%d, left=%d", current_min, min_size_y, min_top, min_left);
  render_atlas(min_layers, min_font_bitmap, &atlas_min, min_size_y, min_top + offset, min_left, ATLAS_NUM_X, ATLAS_NUM_Y_ML);

  if (config_data.is_enable_date)
  {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "date: %d, size=%d, top=%d, left=%d", current_date, date_size, date_top, date_left);
    render_atlas(date_layers, date_font_bitmap, &atlas_date, date_size, date_top + offset, date_left, ATLAS_NUM_X, ATLAS_NUM_Y_S);
  }

  if (config_data.is_enable_month)
  {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "month: %d, size=%d, top=%d, left=%d", current_month, month_size, month_top, month_left);
    render_atlas(month_layers, month_font_bitmap, &atlas_month, month_size, month_top + offset, month_left, ATLAS_NUM_X, ATLAS_NUM_Y_S);
  }
}

// -----------------------------------------------------------------------------
// star
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
static Animation* anim = NULL;

static float prev_ratio, max_spawn_ratio, spawn_timer;
static bool time_refreshed;

static void anim_setup(struct Animation* animation)
{
  prev_ratio = 0.f;
  max_spawn_ratio = (STAR_TRANSITION_PERIOD - ((MAX_SCALE - 1.f) / SCALE_SPEED)) / STAR_TRANSITION_PERIOD;
  spawn_timer = 0.f;
  time_refreshed = false;
}

static void anim_update(struct Animation* animation, const AnimationProgress time_normalized)
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

      graphics_context_set_fill_color(ctx, config_data.star_color);
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
}

static void start_star_transition()
{
#ifdef PBL_PLATFORM_APLITE
  if (anim) animation_destroy(anim);
#endif

  anim = animation_create();
  animation_set_delay(anim, 0);
  animation_set_duration(anim, (int)(STAR_TRANSITION_PERIOD * 1000));
  animation_set_implementation(anim, &anim_impl);

  animation_schedule(anim);
}

static void deinit_star_transition()
{
#ifdef PBL_PLATFORM_APLITE
  if (anim) animation_destroy(anim);
#endif

  layer_destroy(star_layer);
  gpath_destroy(star_path);
  free(curr_star_path_info.points);
}

// -----------------------------------------------------------------------------
// tick proccess
// -----------------------------------------------------------------------------

static void handle_min_tick(struct tm* time, TimeUnits units_changed)
{
  int now_hr = time->tm_hour;
  int now_min = time->tm_min;
  int now_date = time->tm_mday;
  int now_month = time->tm_mon;

#ifdef DEBUG
  now_hr = debug_hour;
  now_min = debug_min;
  now_date = debug_date;
  now_month = debug_month;
  
  debug_min++;
  if (debug_min >= 60)
  {
    debug_min = 0;
    debug_hour++;
  }
  if (debug_hour >= 24)
  {
    debug_hour = 0;
    debug_date++;
  }
  if (debug_date >= 32)
  {
    debug_date = 1;
    debug_month++;
  }
  if (debug_month >= 12)
  {
    debug_month = 0;
  }
#endif

  if (current_hr != now_hr || current_min != now_min || current_date != now_date || current_month != now_month)
  {
    current_hr = now_hr;
    current_min = now_min;
    current_date = now_date;
    current_month = now_month;

    need_refresh_time = true;
  }

  start_star_transition();
}

// -----------------------------------------------------------------------------

static void set_color(GBitmap* image, GColor color, GColor back_color){
  if(image == NULL) return;

#ifdef PBL_COLOR
  if (gcolor_equal(color, GColorBlack) && gcolor_equal(back_color, GColorWhite))
  {
    replace_gbitmap_color(GColorBlack, GColorRed, image, NULL);
    replace_gbitmap_color(GColorWhite, color, image, NULL);
    replace_gbitmap_color(GColorRed, back_color, image, NULL);
  }
  else if (gcolor_equal(color, GColorBlack))
  {
    replace_gbitmap_color(GColorBlack, back_color, image, NULL);
    replace_gbitmap_color(GColorWhite, color, image, NULL);
  }
  else
  {
    replace_gbitmap_color(GColorWhite, color, image, NULL);
    replace_gbitmap_color(GColorBlack, back_color, image, NULL);
  }
#endif
}

static void set_time_bitmap_comp_mode(GCompOp mode)
{
  for (int i = 0; i < CHAR_MAX_LENGTH; ++i)
  {
    bitmap_layer_set_compositing_mode(hour_layers[i], mode);
    bitmap_layer_set_compositing_mode(min_layers[i], mode);
    bitmap_layer_set_compositing_mode(month_layers[i], mode);
    bitmap_layer_set_compositing_mode(date_layers[i], mode);
  }
}

static void refresh_color_theme()
{
#ifndef PBL_COLOR
  config_data.bg_color = (gcolor_equal(GColorBlack, config_data.bg_color)) ? GColorBlack : GColorWhite;
#endif

  window_set_background_color(window, config_data.bg_color);

  if (font_s_bitmap_date) gbitmap_destroy(font_s_bitmap_date);
  if (font_s_bitmap_month) gbitmap_destroy(font_s_bitmap_month);
  if (font_m_bitmap) gbitmap_destroy(font_m_bitmap);
  if (font_l_bitmap) gbitmap_destroy(font_l_bitmap);

#ifdef PBL_PLATFORM_APLITE
  font_m_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FONT_36);
  font_l_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FONT_48);
  font_s_bitmap_date = gbitmap_create_with_resource(RESOURCE_ID_FONT_24);
  font_s_bitmap_month = font_s_bitmap_date;
#else
  font_m_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FONT_36);
  font_l_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FONT_48);
  font_s_bitmap_date = gbitmap_create_with_resource(RESOURCE_ID_FONT_24);
  font_s_bitmap_month = gbitmap_create_with_resource(RESOURCE_ID_FONT_24);
#endif

#ifdef PBL_COLOR
  set_color(font_s_bitmap_date, config_data.date_color, config_data.bg_color);
  set_color(font_s_bitmap_month, config_data.month_color, config_data.bg_color);
  set_color(font_m_bitmap, config_data.time_color, config_data.bg_color);
  set_color(font_l_bitmap, config_data.time_color, config_data.bg_color);
#else
  set_time_bitmap_comp_mode( (gcolor_equal(GColorBlack, config_data.bg_color)) ? GCompOpAssign : GCompOpSet);
#endif

  refresh_time();
}

// -----------------------------------------------------------------------------
// load resource
// -----------------------------------------------------------------------------

static void window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_width = bounds.size.w;
  window_height = bounds.size.h;

  GRect rect = GRect(0, 0, NUM_M_SIZE, NUM_M_SIZE);

  Layer* layer;
  for (int i = 0; i < CHAR_MAX_LENGTH; ++i)
  {
    hour_layers[i] = bitmap_layer_create(rect);
    layer = bitmap_layer_get_layer(hour_layers[i]);
    layer_add_child(window_layer, layer);
    layer_set_hidden(layer, true);

    min_layers[i] = bitmap_layer_create(rect);
    layer = bitmap_layer_get_layer(min_layers[i]);
    layer_add_child(window_layer, layer);
    layer_set_hidden(layer, true);

    date_layers[i] = bitmap_layer_create(rect);
    layer = bitmap_layer_get_layer(date_layers[i]);
    layer_add_child(window_layer, layer);
    layer_set_hidden(layer, true);

    month_layers[i] = bitmap_layer_create(rect);
    layer = bitmap_layer_get_layer(month_layers[i]);
    layer_add_child(window_layer, layer);
    layer_set_hidden(layer, true);
  }

  set_time_bitmap_comp_mode(GCompOpAssign);

  time_t timestamp = time(NULL);
  struct tm* time = localtime(&timestamp);

  current_hr = time->tm_hour;
  current_min = time->tm_min;
  current_date = time->tm_mday;
  current_month = time->tm_mon;

  init_star_transition(window_layer, &bounds);
  refresh_color_theme();
  tick_timer_service_subscribe(MINUTE_UNIT, handle_min_tick);
}

static void window_unload(Window *window)
{
  deinit_star_transition();

  for (int i = 0; i < CHAR_MAX_LENGTH; ++i)
  {
    bitmap_layer_destroy(hour_layers[i]);
    bitmap_layer_destroy(min_layers[i]);
    bitmap_layer_destroy(date_layers[i]);
    bitmap_layer_destroy(month_layers[i]);
  }

  if (font_s_bitmap_date) gbitmap_destroy(font_s_bitmap_date);
  if (font_s_bitmap_month) gbitmap_destroy(font_s_bitmap_month);
  if (font_m_bitmap) gbitmap_destroy(font_m_bitmap);
  if (font_l_bitmap) gbitmap_destroy(font_l_bitmap);
}

// -----------------------------------------------------------------------------
// config setting
// -----------------------------------------------------------------------------

static unsigned int hex_string_to_uint(const char *hex_string)
{
  unsigned int result = 0;
  const char *p = hex_string;
  unsigned char c;
  while ((c = *p) != 0)
  {
    c = toupper(c);
    result <<= 4;
    if (isdigit(c))
      result += c - '0';
    else if (isxdigit(c))
      result += c - 'A' + 10;
    else
      return 0;

    ++p;
  }

  return result;
}

// static void uint_to_hex_string(unsigned int hex, char* out_hex_string)
// {
//   char *p;
//   unsigned char c;
//   for (int i = 1; i < 7; ++i)
//   {
//     p = &out_hex_string[i];
//     c = (hex >> (6 - i) * 4) & 0x0F;
//     if (c >= 10)
//       *p = 'A' + (c - 10);
//     else
//       *p = '0' + c;
//   }

//   out_hex_string[0] = '#';
//   out_hex_string[7] = '\0';

//   APP_LOG(APP_LOG_LEVEL_DEBUG, "hex_string: %x %s", hex, out_hex_string);
// }

static GColor get_color_from_hex_string(const char* hex_string)
{
  if ('#' == hex_string[0])
    hex_string += 1;
  else if (0 == strncmp(hex_string, "0x", 2))
    hex_string += 2;

  unsigned int hex = hex_string_to_uint(hex_string);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "color %s, %x", hex_string, hex);

#ifdef PBL_COLOR
  GColor color = GColorFromHEX(hex);
  return color;
#else
  return hex > 0 ? GColorWhite : GColorBlack;
#endif
}

// static void get_hex_string_from_color(GColor color, char* out_hex_string)
// {
// #ifdef PBL_COLOR
//   uint8_t r = ((GColor8)color).r / 3.f * 255;
//   uint8_t g = ((GColor8)color).g / 3.f * 255;
//   uint8_t b = ((GColor8)color).b / 3.f * 255;
//   unsigned int hex = (r << 16) + (g << 8) + (b);
//   uint_to_hex_string(hex, out_hex_string);
// #else
//   uint_to_hex_string(GColorWhite == color ? 0xFFFFFF : 0x000000, out_hex_string);
// #endif
// }

static void inbox_received_callback(DictionaryIterator *iterator, void *context)
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Inbox receive success!");

  bool need_refresh_color = false;

  Tuple *t = dict_read_first(iterator);
  while (t)
  {
    switch(t->key)
    {
    case MSG_CONFIG_BG_COLOR:
      APP_LOG(APP_LOG_LEVEL_INFO, "CONFIG_BG_COLOR: %s", t->value->cstring);
      config_data.bg_color = get_color_from_hex_string(t->value->cstring);
      need_refresh_color = true;
      break;

    case MSG_CONFIG_STAR_COLOR:
      APP_LOG(APP_LOG_LEVEL_INFO, "CONFIG_STAR_COLOR: %s", t->value->cstring);
      config_data.star_color = get_color_from_hex_string(t->value->cstring);
      need_refresh_color = true;
      break;

    case MSG_CONFIG_TIME_COLOR:
      APP_LOG(APP_LOG_LEVEL_INFO, "CONFIG_TXT_COLOR: %s", t->value->cstring);
      config_data.time_color = get_color_from_hex_string(t->value->cstring);
      need_refresh_color = true;
      break;

    case MSG_CONFIG_DATE_COLOR:
      APP_LOG(APP_LOG_LEVEL_INFO, "MSG_CONFIG_DATE_COLOR: %s", t->value->cstring);
      config_data.date_color = get_color_from_hex_string(t->value->cstring);
      need_refresh_color = true;
      break;

    case MSG_CONFIG_MONTH_COLOR:
      APP_LOG(APP_LOG_LEVEL_INFO, "MSG_CONFIG_MONTH_COLOR: %s", t->value->cstring);
      config_data.month_color = get_color_from_hex_string(t->value->cstring);
      need_refresh_color = true;
      break;

    case MSG_CONFIG_IS_ENABLE_DATE:
      APP_LOG(APP_LOG_LEVEL_INFO, "MSG_CONFIG_IS_ENABLE_DATE: %d", t->value->uint8);
      config_data.is_enable_date = (t->value->uint8 == 0) ? false : true;
      need_refresh_color = true;
      break;

    case MSG_CONFIG_IS_ENABLE_MONTH:
      APP_LOG(APP_LOG_LEVEL_INFO, "MSG_CONFIG_IS_ENABLE_MONTH: %d", t->value->uint8);
      config_data.is_enable_month = (t->value->uint8 == 0) ? false : true;
      need_refresh_color = true;
      break;

    case MSG_CONFIG_IS_USE_AMPM:
      APP_LOG(APP_LOG_LEVEL_INFO, "MSG_CONFIG_IS_USE_AMPM: %d", t->value->uint8);
      config_data.is_use_ampm = t->value->uint8;
      need_refresh_color = true;
      break;

    case MSG_CONFIG_IS_USE_LUNAR:
      APP_LOG(APP_LOG_LEVEL_INFO, "MSG_CONFIG_IS_USE_LUNAR: %d", t->value->uint8);
#ifdef PBL_PLATFORM_APLITE
      config_data.is_use_lunar = false;
#else
      config_data.is_use_lunar = (t->value->uint8 == 0) ? false : true;
#endif
      need_refresh_color = true;
      break;

    case MSG_CONFIG_IS_USE_PREFIX:
      APP_LOG(APP_LOG_LEVEL_INFO, "MSG_CONFIG_IS_USE_PREFIX: %d", t->value->uint8);
      config_data.is_use_prefix = (t->value->uint8 == 0) ? false : true;
      need_refresh_color = true;
      break;

    case MSG_CONFIG_IS_USE_FORMAL:
      APP_LOG(APP_LOG_LEVEL_INFO, "MSG_CONFIG_IS_USE_FORMAL: %d", t->value->uint8);
#ifdef PBL_PLATFORM_APLITE
      config_data.is_use_formal = false;
#else
      config_data.is_use_formal = (t->value->uint8 == 0) ? false : true;
#endif
      need_refresh_color = true;
      break;

    case MSG_CONFIG_DATE_POSITION_TYPE:
      APP_LOG(APP_LOG_LEVEL_INFO, "MSG_CONFIG_DATE_POSITION_TYPE: %d", t->value->uint8);
      config_data.date_position_type = t->value->uint8;
      need_refresh_color = true;
      break;

    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    t = dict_read_next(iterator);
  }

  if (need_refresh_color)
  {
    refresh_color_theme();
    start_star_transition();
    save_config();
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context)
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init_app_message()
{
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

static void init(void)
{
  init_config();
  save_config();
  init_app_message();

  window = window_create();
  window_set_background_color(window, config_data.bg_color);
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
  app_event_loop();
  deinit();
}
