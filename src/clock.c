#include "core2forAWS.h"
#include <sys/time.h>
#include <time.h>

void clock_task(void* pvParameters){
  xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
  lv_obj_t* time_label = lv_label_create((lv_obj_t*)pvParameters, NULL);
  lv_label_set_text(time_label, "00:00:00");
  lv_label_set_align(time_label, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(time_label, NULL, LV_ALIGN_IN_TOP_LEFT, 4, 10);
  xSemaphoreGive(xGuiSemaphore);

  for(;;){
    time_t tmp;
    struct tm now;
    tmp = time(0);
    gmtime_r(&tmp, &now);

    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", &now);

    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    lv_label_set_text(time_label, buf);
    xSemaphoreGive(xGuiSemaphore);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  vTaskDelete(NULL); // Should never get to here...
}

void get_rtctime() {
  // Set initial time based on RTC clock
  rtc_date_t rtc_time;
  struct timeval now = {};
  struct tm rtct = {};
  BM8563_GetTime(&rtc_time);
  rtct.tm_year = rtc_time.year - 1900;
  rtct.tm_mon = rtc_time.month;
  rtct.tm_mday = rtc_time.day;
  rtct.tm_hour = rtc_time.hour;
  rtct.tm_min = rtc_time.minute;
  rtct.tm_sec = rtc_time.second;
  now.tv_sec = mktime(&rtct);
  settimeofday(&now, 0);
}
