#include "core2forAWS.h"

void battery_task(void* pvParameters){
  xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
  lv_obj_t* battery_label = lv_label_create((lv_obj_t*)pvParameters, NULL);
  lv_label_set_text(battery_label, LV_SYMBOL_BATTERY_FULL);
  lv_label_set_recolor(battery_label, true);
  lv_label_set_align(battery_label, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(battery_label, (lv_obj_t*)pvParameters, LV_ALIGN_IN_TOP_RIGHT, -50, 10);
  lv_obj_t* charge_label = lv_label_create(battery_label, NULL);
  lv_label_set_recolor(charge_label, true);
  lv_label_set_text(charge_label, "");
  lv_obj_align(charge_label, battery_label, LV_ALIGN_CENTER, -4, 0);
  xSemaphoreGive(xGuiSemaphore);

  for(;;){
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    float battery_voltage = Core2ForAWS_PMU_GetBatVolt();
    if(battery_voltage >= 4.100){
      lv_label_set_text_fmt(battery_label, "#0ab300 " LV_SYMBOL_BATTERY_FULL "# %.2fV", battery_voltage);
    } else if(battery_voltage >= 3.95){
      lv_label_set_text_fmt(battery_label, "#0ab300 " LV_SYMBOL_BATTERY_3 "# %.2fV", battery_voltage);
    } else if(battery_voltage >= 3.80){
      lv_label_set_text_fmt(battery_label, "#ff9900 " LV_SYMBOL_BATTERY_2 "# %.2fV", battery_voltage);
    } else if(battery_voltage >= 3.25){
      lv_label_set_text_fmt(battery_label, "#ff0000 " LV_SYMBOL_BATTERY_1 "# %.2fV", battery_voltage);
    } else{
      lv_label_set_text_fmt(battery_label, "#ff0000 " LV_SYMBOL_BATTERY_EMPTY "# %.2fV", battery_voltage);
    }

    if(Core2ForAWS_PMU_GetBatCurrent() >= 0.00){
      lv_label_set_text(charge_label, "#0000cc " LV_SYMBOL_CHARGE "#");
    } else{
      lv_label_set_text(charge_label, "");
    }
    xSemaphoreGive(xGuiSemaphore);
    vTaskDelay(pdMS_TO_TICKS(200));
  }

  vTaskDelete(NULL); // Should never get to here...
}
