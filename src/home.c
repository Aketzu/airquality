
#include "core2forAWS.h"

#define TAG "I2C"

uint8_t SensCrc(uint8_t data[2]) {
  uint8_t crc = 0xFF;
  for(int i = 0; i < 2; i++) {
    crc ^= data[i];
    for(uint8_t bit = 8; bit > 0; --bit) {
      if(crc & 0x80) {
        crc = (crc << 1) ^ 0x31u;
      } else {
        crc = (crc << 1);
      }
    }
  }
  return crc;
}

#undef PORT_A_I2C_STANDARD_BAUD
#define PORT_A_I2C_STANDARD_BAUD 50000

void i2c_task(void *par) {
  lv_obj_t *label = (lv_obj_t*)par;
  esp_err_t err;


  I2CDevice_t sht40 = Core2ForAWS_Port_A_I2C_Begin(0x44, PORT_A_I2C_STANDARD_BAUD);

  // Reset
  err = Core2ForAWS_Port_A_I2C_Write(sht40, I2C_NO_REG, (uint8_t*) "\x94", 1);
  vTaskDelay(pdMS_TO_TICKS(1));

  // Read serial
  err = Core2ForAWS_Port_A_I2C_Write(sht40, I2C_NO_REG, (uint8_t*) "\x89", 1);
  vTaskDelay(pdMS_TO_TICKS(1));

  uint8_t shtser[6] = {};
  err = Core2ForAWS_Port_A_I2C_Read(sht40, I2C_NO_REG, shtser, sizeof(shtser));
  if(!err){
    if (SensCrc(&shtser[0]) != shtser[2] || SensCrc(&shtser[3]) != shtser[5]) {
      ESP_LOGI("SHT40", "CRC error in serial");
    }
    ESP_LOGI("SHT40", "Sensor serial %02x %02x %02x %02x", shtser[0], shtser[1], shtser[3], shtser[4]);
  }

  vTaskDelay(pdMS_TO_TICKS(50));


  /*
  I2CDevice_t sgp40 = Core2ForAWS_Port_A_I2C_Begin(0x59, PORT_A_I2C_STANDARD_BAUD);

  // Reset
  err = Core2ForAWS_Port_A_I2C_Write(sgp40, I2C_NO_REG, (uint8_t*) "\x00\x06", 2);
  vTaskDelay(pdMS_TO_TICKS(1));

  // Read serial
  err = Core2ForAWS_Port_A_I2C_Write(sgp40, I2C_NO_REG, (uint8_t*) "\x36\x82", 2);
  vTaskDelay(pdMS_TO_TICKS(1));
  uint8_t sgpser[9] = {};
  err = Core2ForAWS_Port_A_I2C_Read(sgp40, I2C_NO_REG, sgpser, sizeof(sgpser));
  if(!err){
    if (SensCrc(&sgpser[0]) != sgpser[2] || SensCrc(&sgpser[3]) != sgpser[5] || SensCrc(&sgpser[6]) != sgpser[8]) {
      ESP_LOGI("SGP40", "CRC error in serial");
    }
    ESP_LOGI("SGP40", "Sensor serial %02x %02x %02x %02x %02x %02x", sgpser[0], sgpser[1], sgpser[3], sgpser[4], sgpser[6], sgpser[7]);
  }
  */


  I2CDevice_t sps30 = Core2ForAWS_Port_A_I2C_Begin(0x69, PORT_A_I2C_STANDARD_BAUD);
  // Wakeup
  err = Core2ForAWS_Port_A_I2C_Write(sps30, I2C_NO_REG, (uint8_t*) "\x11\x03", 2);
  vTaskDelay(pdMS_TO_TICKS(5));

  // Get serial
  err = Core2ForAWS_Port_A_I2C_Write(sps30, I2C_NO_REG, (uint8_t*) "\xd0\x33", 2);
  vTaskDelay(pdMS_TO_TICKS(1));
  uint8_t spsser[48] = {};
  err = Core2ForAWS_Port_A_I2C_Read(sps30, I2C_NO_REG, spsser, sizeof(spsser));
  if(!err){
    char serial[32] = {};
    char *sp = serial;
    for (int i=0; i<48; i+=3) {
      *sp = spsser[i];
      sp++;
      *sp = spsser[i+1];
      sp++;
    }
    ESP_LOGI("SPS30", "Serial '%s'", serial);
  }

  // Start measurements, report floats
  err = Core2ForAWS_Port_A_I2C_Write(sps30, I2C_NO_REG, (uint8_t*) "\x00\x10\x03\x00\xac", 5);
  vTaskDelay(pdMS_TO_TICKS(20));


  I2CDevice_t scd30 = Core2ForAWS_Port_A_I2C_Begin(0x61, PORT_A_I2C_STANDARD_BAUD);
  // Reset
  //err = Core2ForAWS_Port_A_I2C_Write(scd30, I2C_NO_REG, (uint8_t*) "\xd3\x04", 2);
  //vTaskDelay(pdMS_TO_TICKS(5));

  // Get FW version
  err = Core2ForAWS_Port_A_I2C_Write(scd30, I2C_NO_REG, (uint8_t*) "\xd1\x00", 2);
  vTaskDelay(pdMS_TO_TICKS(1));
  uint8_t scdver[3] = {};
  err = Core2ForAWS_Port_A_I2C_Read(scd30, I2C_NO_REG, scdver, sizeof(scdver));
  if(!err){
    ESP_LOGI("SCD30", "FW ver %d.%d", scdver[0], scdver[1]);
  }

  err = Core2ForAWS_Port_A_I2C_Write(scd30, I2C_NO_REG, (uint8_t*) "\x00\x10\x00\x00\x81", 5);
  vTaskDelay(pdMS_TO_TICKS(5));


  I2CDevice_t gm = Core2ForAWS_Port_A_I2C_Begin(0x08, PORT_A_I2C_STANDARD_BAUD);
  // Warm up sensors
  err = Core2ForAWS_Port_A_I2C_Write(gm, I2C_NO_REG, (uint8_t*) "\xFE", 1);


  vTaskDelay(pdMS_TO_TICKS(50));

  float temp = 0, rh = 0;
  float pms[10] = {};
  float coo[3] = {};
  uint32_t no2 = 0, alc = 0, voc = 0, co = 0;


  for(;;){

    // Read temp & RH
    //err = Core2ForAWS_Port_A_I2C_Write(sht40, I2C_NO_REG, (uint8_t*) "\xfd", 1);
    //err = Core2ForAWS_Port_A_I2C_Write(sht40, I2C_NO_REG, (uint8_t*) "\xf6", 1);
    err = Core2ForAWS_Port_A_I2C_Write(sht40, I2C_NO_REG, (uint8_t*) "\xe0", 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t tmp[6] = {};
    err = Core2ForAWS_Port_A_I2C_Read(sht40, I2C_NO_REG, tmp, sizeof(tmp));
    if(!err){
      if (SensCrc(&tmp[0]) != tmp[2] || SensCrc(&tmp[3]) != tmp[5]) {
        ESP_LOGI("SHT40", "Sensor CRC fail");
      }
      temp = -45 + 175.0*(tmp[0]*256 + tmp[1])/65535;
      rh = -6 + 125.0*(tmp[3]*256 + tmp[4])/65535;
      ESP_LOGI("SHT40", "Temp %.2f, RH %.2f", temp, rh);
    } else {
      ESP_LOGI(TAG, "Read temp? %s", esp_err_to_name(err));
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    //int vocs = 0;
    /*
    // Read VOC
    uint8_t sgpmeas[6] = { 0x26, 0x0f };
    int x;
    x = rh * 65535 / 100;
    sgpmeas[2] = x>>8;
    sgpmeas[3] = x & 0xFF;
    sgpmeas[4] = SensCrc(&sgpmeas[2]);
    x = (temp + 45) * 65535 / 175;
    sgpmeas[5] = x>>8;
    sgpmeas[6] = x & 0xFF;
    sgpmeas[7] = SensCrc(&sgpmeas[5]);

    err = Core2ForAWS_Port_A_I2C_Write(sgp40, I2C_NO_REG, sgpmeas, sizeof(sgpmeas));
    vTaskDelay(pdMS_TO_TICKS(30));

    uint8_t voc[3] = {};
    err = Core2ForAWS_Port_A_I2C_Read(sgp40, I2C_NO_REG, tmp, sizeof(tmp));
    if(!err){
      if (SensCrc(&voc[0]) != voc[2]) {
        ESP_LOGI("SGP40", "Sensor CRC fail");
      }
      vocs = (voc[0]<<8) + voc[1];
      ESP_LOGI("SGP40", "Voc %d", vocs);
    }
    */

    vTaskDelay(pdMS_TO_TICKS(20));

    // Read PM2.5
    err = Core2ForAWS_Port_A_I2C_Write(sps30, I2C_NO_REG, (uint8_t*) "\x03\x00", 2);
    vTaskDelay(pdMS_TO_TICKS(1));

    uint8_t atmp[60] = {};
    err = Core2ForAWS_Port_A_I2C_Read(sps30, I2C_NO_REG, atmp, sizeof(atmp));
    if (!err) {
      for (int i = 0; i<60; i+=6) {
        // After every 2 bytes the checksum of previous two bytes is transferred
        uint8_t vt[4];
        vt[0] = atmp[i+4];
        vt[1] = atmp[i+3];
        vt[2] = atmp[i+1];
        vt[3] = atmp[i+0];

        pms[i/6] = *(float*)vt;
      }
      ESP_LOGI("SPS30", "PMS: %.2f %.2f %.2f %.2f %.2f", pms[0], pms[1], pms[2], pms[3], pms[4]);
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    // Read CO2 status
    err = Core2ForAWS_Port_A_I2C_Write(scd30, I2C_NO_REG, (uint8_t*) "\x02\x02", 2);
    vTaskDelay(pdMS_TO_TICKS(3));
    uint8_t st[2] = {};
    err = Core2ForAWS_Port_A_I2C_Read(scd30, I2C_NO_REG, st, sizeof(st));
    if (!err) {
      ESP_LOGI("SCD30", "Data ready: %d %d", st[0], st[1]);

      if (st[0] || st[1]) {
        err = Core2ForAWS_Port_A_I2C_Write(scd30, I2C_NO_REG, (uint8_t*) "\x03\x00", 2);
        vTaskDelay(pdMS_TO_TICKS(5));
        uint8_t coval[18] = {};
        err = Core2ForAWS_Port_A_I2C_Read(scd30, I2C_NO_REG, coval, sizeof(coval));
        if (!err) {
          for (int i = 0; i<18; i+=6) {
            // After every 2 bytes the checksum of previous two bytes is transferred
            uint8_t vt[4];
            vt[0] = coval[i+4];
            vt[1] = coval[i+3];
            vt[2] = coval[i+1];
            vt[3] = coval[i+0];
            coo[i/6] = *(float*)vt;
          }

          ESP_LOGI("SCD30", "CO2 %.2f, T %.2f, RH %.2f", coo[0], coo[1], coo[2]);
        }
      }
    }

    // Gas sensors
    err = Core2ForAWS_Port_A_I2C_Write(gm, I2C_NO_REG, (uint8_t*) "\x01", 2);
    err = Core2ForAWS_Port_A_I2C_Read(gm, I2C_NO_REG, &no2, sizeof(no2));
    err = Core2ForAWS_Port_A_I2C_Write(gm, I2C_NO_REG, (uint8_t*) "\x03", 2);
    err = Core2ForAWS_Port_A_I2C_Read(gm, I2C_NO_REG, &alc, sizeof(alc));
    err = Core2ForAWS_Port_A_I2C_Write(gm, I2C_NO_REG, (uint8_t*) "\x05", 2);
    err = Core2ForAWS_Port_A_I2C_Read(gm, I2C_NO_REG, &voc, sizeof(voc));
    err = Core2ForAWS_Port_A_I2C_Write(gm, I2C_NO_REG, (uint8_t*) "\x07", 2);
    err = Core2ForAWS_Port_A_I2C_Read(gm, I2C_NO_REG, &co, sizeof(co));
    ESP_LOGI("GMx02B", "NO2 %u, Alc %u, VOC %u, CO %u", no2, alc, voc, co);


    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    lv_label_set_text_fmt(label, "Temp %.2f/%.2f, RH %.2f/%.2f\nCO2 %.2f\n"
        "PM1.0 %.2f, PM2.5 %.2f,\nPM4.0 %.2f, PM10 %.2f, avg %.2f\nNO2 %.2f, Alc %.2f, VOC %.2f\nCO %.2f",
        temp, coo[1], rh, coo[2], coo[0], pms[0], pms[1], pms[2], pms[3], pms[9],
        no2*3.3/1023, alc*3.3/1023, voc*3.3/1023, co*3.3/1023);
    xSemaphoreGive(xGuiSemaphore);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  vTaskDelete(NULL);
}

void display_home_tab(lv_obj_t* tv){
  xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);   // Takes (blocks) the xGuiSemaphore mutex from being read/written by another task.

  lv_obj_t* home_tab = lv_tabview_add_tab(tv, "Home");   // Create a tab

  /* Create the title within the tab */
  static lv_style_t title_style;
  lv_style_init(&title_style);
  lv_style_set_text_font(&title_style, LV_STATE_DEFAULT, LV_THEME_DEFAULT_FONT_TITLE);
  lv_style_set_text_color(&title_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);

  lv_obj_t* tab_title_label = lv_label_create(home_tab, NULL);
  lv_obj_add_style(tab_title_label, LV_OBJ_PART_MAIN, &title_style);
  lv_label_set_static_text(tab_title_label, "AirQuality");
  lv_label_set_align(tab_title_label, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(tab_title_label, home_tab, LV_ALIGN_IN_TOP_MID, 0, 0);

  lv_obj_t* body_label = lv_label_create(home_tab, NULL);
  lv_label_set_long_mode(body_label, LV_LABEL_LONG_BREAK);
  lv_label_set_static_text(body_label, "Measurements TBD");
  lv_obj_set_width(body_label, 280);
  lv_obj_align(body_label, home_tab, LV_ALIGN_CENTER, 0, -30);

  /*
  lv_obj_t* arrow_label = lv_label_create(home_tab, NULL);
  lv_label_set_recolor(arrow_label, true);
  size_t arrow_buf_len = snprintf (NULL, 0,
      "#ff9900 %1$s       %1$s       %1$s#       #232f3e SWIPE #      #ff9900 %1$s       %1$s       %1$s#       #232f3e SWIPE #",
      LV_SYMBOL_LEFT);
  char arrow_buf[++arrow_buf_len];
  snprintf (arrow_buf, arrow_buf_len,
      "#ff9900 %1$s       %1$s       %1$s#       #232f3e SWIPE #      #ff9900 %1$s       %1$s       %1$s#       #232f3e SWIPE #",
      LV_SYMBOL_LEFT);
  lv_label_set_text(arrow_label, arrow_buf);
  lv_label_set_long_mode(arrow_label, LV_LABEL_LONG_SROLL_CIRC);
  lv_label_set_anim_speed(arrow_label, 50);
  lv_obj_set_size(arrow_label, 290, 20);
  lv_label_set_align(arrow_label, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(arrow_label, home_tab, LV_ALIGN_IN_BOTTOM_MID, 0 , -40);
  */
  xSemaphoreGive(xGuiSemaphore);

  xTaskCreatePinnedToCore(i2c_task, "I2CTask", 4096, (void*) body_label, 1, NULL, 1);

  ESP_LOGI("Home", "Init done");
}
