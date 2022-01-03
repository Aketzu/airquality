#include <arpa/inet.h>

#include "core2forAWS.h"
#include "sensors.h"

#include "sensirion_voc_algorithm.h"

uint8_t SensCrc(uint8_t data[2]) {
  uint8_t crc = 0xFF;
  for (int i = 0; i < 2; i++) {
    crc ^= data[i];
    for (uint8_t bit = 8; bit > 0; --bit) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31u;
      } else {
        crc = (crc << 1);
      }
    }
  }
  return crc;
}

struct sensor {
  char name[20];
  I2CDevice_t dev;
};

struct sensor *init_sensor(int i2c_addr, char *name) {
  struct sensor *sens = calloc(1, sizeof(struct sensor));
  sens->dev = Core2ForAWS_Port_A_I2C_Begin(i2c_addr, 50000);
  //sens->dev = Core2ForAWS_Port_A_I2C_Begin(i2c_addr, 100000);

  strncpy(sens->name, name, sizeof(sens->name));

  return sens;
}

void free_sensor(struct sensor *sens) {
  Core2ForAWS_Port_A_I2C_Close(sens->dev);
  free(sens);
}

struct sensordata sensors = {};

esp_err_t sensor_send(struct sensor *sens, void *data, int data_len, int wait,
                      void *resp, int resp_len) {
  esp_err_t err;

  if ((data_len > 0 && !data) || (resp_len > 0 && !resp)) {
    return ESP_ERR_INVALID_ARG;
  }

  err = Core2ForAWS_Port_A_I2C_Write(sens->dev, I2C_NO_REG, data, data_len);
  if (err) {
    ESP_LOGW(sens->name, "Write failed: %s", esp_err_to_name(err));
    return err;
  }
  if (wait > 0) {
    vTaskDelay(pdMS_TO_TICKS(wait));
  }

  if (resp_len < 1) {
    return ESP_OK;
  }

  uint8_t *retbuf = calloc(1, resp_len);
  err = Core2ForAWS_Port_A_I2C_Read(sens->dev, I2C_NO_REG, retbuf, resp_len);
  if (err) {
    ESP_LOGW(sens->name, "Read failed: %s", esp_err_to_name(err));
    return err;
  }

  // If Sensirion do CRC checks
  if (sens->name[0] == 'S') { // Works for me
    uint8_t *rp = resp;
    for (int i = 0; i < resp_len; i += 3) {
      if (SensCrc(&retbuf[i]) != retbuf[i + 2]) {
        ESP_LOGE(sens->name, "CRC error pos %d: 0x%02x != CRC(0x%02x, 0x%02x)",
                 i, retbuf[i + 2], retbuf[i], retbuf[i + 1]);
        free(retbuf);
        return ESP_ERR_INVALID_CRC;
      }
      *rp++ = retbuf[i];
      *rp++ = retbuf[i + 1];
    }
  } else {
    memcpy(resp, retbuf, resp_len);
  }

  free(retbuf);
  return ESP_OK;
}

void sensors_task(void *par) {
  (void)par;
  //lv_obj_t *label = (lv_obj_t *)par;
  esp_err_t err;

  VocAlgorithmParams voc_algorithm_params;
  int32_t voc_index = 0;
  VocAlgorithm_init(&voc_algorithm_params);

  struct sensor *sht40 = init_sensor(0x44, "SHT40");
  // Reset
  sensor_send(sht40, "\x94", 1, 10, 0, 0);

  // Read serial
  uint8_t shtser[4] = {};
  err = sensor_send(sht40, "\x89", 1, 1, shtser, 6);
  if (!err) {
    ESP_LOGI("SHT40", "Sensor serial %02x %02x %02x %02x", shtser[0], shtser[1],
             shtser[2], shtser[3]);
  }

  struct sensor *sgp40 = init_sensor(0x59, "SGP40");
  // Reset
  // sensor_send(sgp40, "\x00\x06", 2, 1, 0, 0);

  // Read serial
  uint8_t sgpser[6] = {};
  err = sensor_send(sgp40, "\x36\x82", 2, 1, sgpser, 9);
  if (!err) {
    ESP_LOGI("SGP40", "Sensor serial %02x %02x %02x %02x %02x %02x", sgpser[0],
             sgpser[1], sgpser[2], sgpser[3], sgpser[4], sgpser[5]);
  }

  struct sensor *sps30 = init_sensor(0x69, "SPS30");
  // Wakup
  sensor_send(sps30, "\x11\x03", 2, 5, 0, 0);

  // Read serial
  char spsser[32] = {};
  err = sensor_send(sps30, "\xd0\x33", 2, 1, spsser, 48);
  if (!err) {
    ESP_LOGI("SPS30", "Serial '%s'", spsser);
  }

  // Start measurements, report floats
  sensor_send(sps30, "\x00\x10\x03\x00\xac", 5, 20, 0, 0);

  struct sensor *scd30 = init_sensor(0x61, "SCD30");
  // Reset
  // sensor_send(scd30, "\xd3\x04", 2, 5, 0, 0);

  // Get FW version
  uint8_t scdver[2] = {};
  err = sensor_send(scd30, "\xd1\x00", 2, 1, scdver, 3);
  if (!err) {
    ESP_LOGI("SCD30", "FW ver %d.%d", scdver[0], scdver[1]);
  }

  // Start measurements
  sensor_send(scd30, "\x00\x10\x00\x00\x81", 5, 5, 0, 0);

  struct sensor *gm = init_sensor(0x08, "GMx02B");
  // Warm up sensors
  sensor_send(gm, "\xfe", 1, 0, 0, 0);

  vTaskDelay(pdMS_TO_TICKS(50));


  for (;;) {
    // Start measurements once per second
    struct timeval now;
    gettimeofday(&now, 0);
    vTaskDelay(pdMS_TO_TICKS(1000 - now.tv_usec / 1000));


    // SHT40, Read temp & RH
    uint16_t temps[2] = {};
    // Precision: e0, f6, fd (highest)
    err = sensor_send(sht40, "\xfd", 1, 10, temps, 6);
    if (!err) {
      sensors.temperature = -45 + 175.0 * ntohs(temps[0]) / 65535;
      sensors.humidity = -6 + 125.0 * ntohs(temps[1]) / 65535;
      sensors.temperature_upd = time(0);
      ESP_LOGI("SHT40", "Temp %.2f, RH %.2f", sensors.temperature, sensors.humidity);
    }


    // SPS30, Read PM2.5
    uint32_t atmp[10] = {};
    err = sensor_send(sps30, "\x03\x00", 2, 1, atmp, 60);
    if (!err) {
      float pms[10] = {};
      // Reverse byte order and cast to floats
      for (int i = 0; i < 10; i++) {
        atmp[i] = ntohl(atmp[i]);
        pms[i] = *(float *)&atmp[i];
      }

      sensors.mass_pm10 = pms[0];
      sensors.mass_pm25 = pms[1];
      sensors.mass_pm40 = pms[2];
      sensors.mass_pm100 = pms[3];
      sensors.num_pm5 = pms[4];
      sensors.num_pm10 = pms[5];
      sensors.num_pm25 = pms[6];
      sensors.num_pm40 = pms[7];
      sensors.num_pm100 = pms[8];
      sensors.pm_avg_sz = pms[9];
      sensors.pm_upd = time(0);

      ESP_LOGI("SPS30",
               "Mass: %.2f %.2f %.2f %.2f, Num: %.2f %.2f %.2f %.2f %.2f, "
               "Size: %.2f",
               pms[0], pms[1], pms[2], pms[3], pms[4], pms[5], pms[6], pms[7],
               pms[8], pms[9]);
    }


    // SGP40, Read VOC
    uint8_t sgpmeas[8] = {0x26, 0x0f};
    int x;
    x = sensors.humidity * 65535 / 100;
    sgpmeas[2] = x >> 8;
    sgpmeas[3] = x & 0xFF;
    sgpmeas[4] = SensCrc(&sgpmeas[2]);
    x = (sensors.temperature + 45) * 65535 / 175;
    sgpmeas[5] = x >> 8;
    sgpmeas[6] = x & 0xFF;
    sgpmeas[7] = SensCrc(&sgpmeas[5]);

    //uint8_t sgpmeas2[8] = {0x26, 0x0f, 0x80, 0x00, 0xa2, 0x66, 0x66, 0x93};
    uint16_t voctmp = 0;
    err = sensor_send(sgp40, sgpmeas, 8, 50, &voctmp, 3);
    if (!err) {
      sensors.voc_raw = ntohs(voctmp);
      VocAlgorithm_process(&voc_algorithm_params, sensors.voc_raw, &voc_index);
      sensors.voc_index = voc_index;
      sensors.voc_upd = time(0);
      ESP_LOGI("SGP40", "Voc raw %u, index %u", (int32_t) sensors.voc_raw, sensors.voc_index);
    }


    // SCD30, Read CO2 status + Temp/Humidity
    uint16_t scdstatus = 0;
    err = sensor_send(scd30, "\x02\x02", 2, 3, &scdstatus, 3);
    if (!err && scdstatus) {
      uint32_t cotmp[3] = {};
      err = sensor_send(scd30, "\x03\x00", 2, 5, cotmp, 18);

      if (!err) {
        float coo[3] = {};
        // Reverse byte order and cast to floats
        for (int i = 0; i < 10; i++) {
          cotmp[i] = ntohl(cotmp[i]);
          coo[i] = *(float *)&cotmp[i];
        }

        sensors.co2 = coo[0];
        sensors.co2_upd = time(0);

        ESP_LOGI("SCD30", "CO2 %.2f, T %.2f, RH %.2f", coo[0], coo[1], coo[2]);
      }
    }

    // GMx02B, Gas sensors
    uint32_t no2 = 0, alc = 0, voc = 0, co = 0;
    err = sensor_send(gm, "\x01", 1, 0, &no2, sizeof(no2));
    if (!err) {
      sensors.gm_no2 = no2;
    }
    err = sensor_send(gm, "\x03", 1, 0, &alc, sizeof(alc));
    if (!err) {
      sensors.gm_alc = alc;
    }
    err = sensor_send(gm, "\x05", 1, 0, &voc, sizeof(voc));
    if (!err) {
      sensors.gm_voc = voc;
    }
    err = sensor_send(gm, "\x07", 1, 0, &co, sizeof(co));
    if (!err) {
      sensors.gm_co = co;
      sensors.gm_upd = time(0);
      ESP_LOGI("GMx02B", "NO2 %u, Alc %u, VOC %u, CO %u", no2, alc, voc, co);
    }

    /*
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    lv_label_set_text_fmt(
        label,
        "Temp %.2f/%.2f, RH %.2f/%.2f\nCO2 %.2f\n"
        "PM1.0 %.2f, PM2.5 %.2f,\nPM4.0 %.2f, PM10 %.2f, avg %.2f\nNO2 %.2f, "
        "Alc %.2f, VOC %.2f\nCO %.2f, Voc raw %u -> %d",
        temp, coo[1], rh, coo[2], coo[0], pms[0], pms[1], pms[2], pms[3],
        pms[9], no2 * 3.3 / 1023, alc * 3.3 / 1023, voc * 3.3 / 1023,
        co * 3.3 / 1023, vocs, voc_index);
    xSemaphoreGive(xGuiSemaphore);
    */
  }

  vTaskDelete(NULL);
}
