#include "core2forAWS.h"
#include "sensors.h"

struct meters {
  lv_obj_t *temp;
  lv_obj_t *hum;
  lv_obj_t *co2;
  lv_obj_t *voci;

  lv_obj_t *mpm10;
  lv_obj_t *mpm25;
  lv_obj_t *mpm40;
  lv_obj_t *mpm100;

  lv_obj_t *no2;
  lv_obj_t *alc;
  lv_obj_t *voc;
  lv_obj_t *co;
};

static struct meters meters = {};

void update_home(void* x) {
  (void)x;

  for (;;) {
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    lv_label_set_text_fmt(meters.temp, "%.2f", sensors.temperature);
    lv_label_set_text_fmt(meters.hum, "%.2f", sensors.humidity);
    lv_label_set_text_fmt(meters.co2, "%.0f", sensors.co2);
    lv_label_set_text_fmt(meters.voci, "%d", sensors.voc_index);

    lv_label_set_text_fmt(meters.mpm10, "%.2f", sensors.mass_pm10);
    lv_label_set_text_fmt(meters.mpm25, "%.2f", sensors.mass_pm25);
    lv_label_set_text_fmt(meters.mpm40, "%.2f", sensors.mass_pm40);
    lv_label_set_text_fmt(meters.mpm100, "%.2f", sensors.mass_pm100);

    lv_label_set_text_fmt(meters.no2, "%d", sensors.gm_no2);
    lv_label_set_text_fmt(meters.alc, "%d", sensors.gm_alc);
    lv_label_set_text_fmt(meters.voc, "%d", sensors.gm_voc);
    lv_label_set_text_fmt(meters.co, "%d", sensors.gm_co);
    xSemaphoreGive(xGuiSemaphore);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  vTaskDelete(NULL);
}

lv_obj_t* make_meter(lv_obj_t *parent, int x, int y, char* label) {
  // Main obj for background
  lv_obj_t* meter_bg = lv_obj_create(parent, NULL);
  lv_obj_align(meter_bg, NULL, LV_ALIGN_IN_TOP_LEFT, x, y);
  lv_obj_set_size(meter_bg, 65, 50);
  lv_obj_set_click(meter_bg, false);

  // Color
  static lv_style_t bg_style;
  lv_style_init(&bg_style);
  lv_style_set_bg_color(&bg_style, LV_STATE_DEFAULT, lv_color_make(236, 216, 218));
  lv_obj_add_style(meter_bg, LV_OBJ_PART_MAIN, &bg_style);

  // Label
  lv_obj_t *meter_label = lv_label_create(meter_bg, NULL);
  lv_label_set_long_mode(meter_label, LV_LABEL_LONG_CROP);
  lv_label_set_align(meter_label, LV_LABEL_ALIGN_CENTER);
  lv_label_set_static_text(meter_label, label);
  lv_obj_set_width(meter_label, 65);
  lv_obj_align(meter_label, meter_bg, LV_ALIGN_IN_TOP_MID, 0, 5);

  // Value
  lv_obj_t *meter_val = lv_label_create(meter_bg, NULL);
  lv_label_set_long_mode(meter_val, LV_LABEL_LONG_CROP);
  lv_label_set_static_text(meter_val, "--.--");
  lv_obj_set_width(meter_val, 65);
  lv_obj_align(meter_val, meter_bg, LV_ALIGN_IN_BOTTOM_MID, 5, -5);

  return meter_val;
}


void display_home_tab(lv_obj_t *tv) {
  xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

  lv_obj_t *home_tab = lv_tabview_add_tab(tv, "Home");

  static lv_style_t title_style;
  lv_style_init(&title_style);
  lv_style_set_text_font(&title_style, LV_STATE_DEFAULT,
                         LV_THEME_DEFAULT_FONT_TITLE);
  lv_style_set_text_color(&title_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);

  lv_obj_t *tab_title_label = lv_label_create(home_tab, NULL);
  lv_obj_add_style(tab_title_label, LV_OBJ_PART_MAIN, &title_style);
  lv_label_set_static_text(tab_title_label, "AirQuality");
  lv_label_set_align(tab_title_label, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(tab_title_label, home_tab, LV_ALIGN_IN_TOP_MID, 0, 10);

  meters.temp = make_meter(home_tab, 16 + 0 * 75, 56, "Temp");
  meters.hum  = make_meter(home_tab, 16 + 1 * 75, 56, "Hum.");
  meters.co2  = make_meter(home_tab, 16 + 2 * 75, 56, "CO2");
  meters.voci = make_meter(home_tab, 16 + 3 * 75, 56, "VOC_i");

  meters.mpm10  = make_meter(home_tab, 16 + 0 * 75, 56 + 60, "PM<1.0");
  meters.mpm25  = make_meter(home_tab, 16 + 1 * 75, 56 + 60, "PM<2.5");
  meters.mpm40  = make_meter(home_tab, 16 + 2 * 75, 56 + 60, "PM<4.0");
  meters.mpm100 = make_meter(home_tab, 16 + 3 * 75, 56 + 60, "PM<10");

  meters.no2 = make_meter(home_tab, 16 + 0 * 75, 56 + 120, "NO2");
  meters.alc = make_meter(home_tab, 16 + 1 * 75, 56 + 120, "Alc");
  meters.voc = make_meter(home_tab, 16 + 2 * 75, 56 + 120, "Voc");
  meters.co  = make_meter(home_tab, 16 + 3 * 75, 56 + 120, "CO");

  xSemaphoreGive(xGuiSemaphore);

  xTaskCreatePinnedToCore(update_home, "homeTask", 4096, 0, 1, 0, 1);
  ESP_LOGI("Home", "Init done");
}
