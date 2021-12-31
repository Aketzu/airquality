#include "nvs_flash.h"

#include "core2forAWS.h"

static void tab_event_cb(lv_obj_t* slider, lv_event_t event);
void display_home_tab(lv_obj_t* tv);

static lv_obj_t* tab_view;

void app_main() {
  ESP_LOGI("Main", "Startup");

  /*
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK( ret );

  esp_log_level_set("gpio", ESP_LOG_NONE);
  esp_log_level_set("ILI9341", ESP_LOG_NONE);
  */

  Core2ForAWS_Init();
  Core2ForAWS_Display_SetBrightness(80); // Last since the display first needs time to finish initializing.

  ESP_LOGI("Main", "Create tabs");

  xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);   // Takes (blocks) the xGuiSemaphore mutex from being read/written by another task.
  lv_obj_t* core2forAWS_obj = lv_obj_create(NULL, NULL); // Create a object to draw all with no parent
  lv_scr_load_anim(core2forAWS_obj, LV_SCR_LOAD_ANIM_MOVE_LEFT, 400, 0, false);   // Animates the loading of core2forAWS_obj as a slide into view from the left
  tab_view = lv_tabview_create(core2forAWS_obj, NULL); // Creates the tab view to display different tabs with different hardware features
  lv_obj_set_event_cb(tab_view, tab_event_cb); // Add a callback for whenever there is an event triggered on the tab_view object (e.g. a left-to-right swipe)
  lv_tabview_set_btns_pos(tab_view, LV_TABVIEW_TAB_POS_NONE);  // Hide the tab buttons so it looks like a clean screen

  xSemaphoreGive(xGuiSemaphore);  // Frees the xGuiSemaphore so that another task can use it. In this case, the higher priority guiTask will take it and then read the values to then display.

  ESP_LOGI("Main", "Init tabs");
  display_home_tab(tab_view);
}

static void tab_event_cb(lv_obj_t* slider, lv_event_t event){
  if(event == LV_EVENT_VALUE_CHANGED) {
    lv_tabview_ext_t* ext = (lv_tabview_ext_t*) lv_obj_get_ext_attr(tab_view);
    const char* tab_name = ext->tab_name_ptr[lv_tabview_get_tab_act(tab_view)];
    ESP_LOGI("Main", "Current Active Tab: %s\n", tab_name);

    /*
    vTaskSuspend(MPU_handle);
    vTaskSuspend(mic_handle);
    vTaskSuspend(FFT_handle);
    vTaskSuspend(wifi_handle);
    vTaskSuspend(touch_handle);
    // vTaskSuspend(led_bar_solid_handle);
    if( led_bar_solid_handle != NULL )
    {
      vTaskSuspend(led_bar_solid_handle);
      // Delete using the copy of the handle.
      vTaskDelete(led_bar_solid_handle);
      // The task is going to be deleted.
      // Set the handle to NULL.
      led_bar_solid_handle = NULL;
    }
    vTaskResume(led_bar_animation_handle);

    if(strcmp(tab_name, CLOCK_TAB_NAME) == 0)
      update_roller_time();
    else if(strcmp(tab_name, MPU_TAB_NAME) == 0)
      vTaskResume(MPU_handle);
    else if(strcmp(tab_name, MICROPHONE_TAB_NAME) == 0){
      vTaskResume(mic_handle);
      vTaskResume(FFT_handle);
    } else if (strcmp(tab_name, LED_BAR_TAB_NAME) == 0){
      vTaskSuspend(led_bar_animation_handle);
      xTaskCreatePinnedToCore(sk6812_solid_task, "sk6812SolidTask", configMINIMAL_STACK_SIZE * 3, NULL, 0, &led_bar_solid_handle, 1);
    }
    else if(strcmp(tab_name, TOUCH_TAB_NAME) == 0){
      reset_touch_bg();
      vTaskResume(touch_handle);
    }
    else if(strcmp(tab_name, WIFI_TAB_NAME) == 0)
      vTaskResume(wifi_handle);
    */
  }
}
