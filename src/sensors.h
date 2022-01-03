#include <stdint.h>
#include <time.h>

struct sensordata {
  float temperature;
  float humidity;
  time_t temperature_upd;

  float mass_pm10;
  float mass_pm25;
  float mass_pm40;
  float mass_pm100;
  float num_pm5;
  float num_pm10;
  float num_pm25;
  float num_pm40;
  float num_pm100;
  float pm_avg_sz;
  time_t pm_upd;

  uint16_t voc_raw;
  int16_t voc_index;
  time_t voc_upd;

  float co2;
  time_t co2_upd;


  uint32_t gm_no2;
  uint32_t gm_alc;
  uint32_t gm_voc;
  uint32_t gm_co;
  time_t gm_upd;
};

extern struct sensordata sensors;

