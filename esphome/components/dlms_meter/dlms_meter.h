#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/log.h"
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#include "esphome/components/uart/uart.h"

#include <dlms_parser/dlms_parser.h>
#if defined(USE_ESP8266_FRAMEWORK_ARDUINO)
#include <dlms_parser/decryption/aes_128_gcm_decryptor_bearssl.h>
#elif defined(USE_ESP32)
#include <esp_idf_version.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
#include <dlms_parser/decryption/aes_128_gcm_decryptor_tfpsa.h>
#else
#include <dlms_parser/decryption/aes_128_gcm_decryptor_mbedtls.h>
#endif
#else
#error "Unsupported platform for dlms_meter"
#endif

#include <vector>

namespace esphome::dlms_meter {

#ifndef DLMS_METER_SENSOR_LIST
#define DLMS_METER_SENSOR_LIST(F, SEP)
#endif

#ifndef DLMS_METER_TEXT_SENSOR_LIST
#define DLMS_METER_TEXT_SENSOR_LIST(F, SEP)
#endif

struct MeterData {
  float voltage_l1 = 0.0f;
  float voltage_l2 = 0.0f;
  float voltage_l3 = 0.0f;
  float current_l1 = 0.0f;
  float current_l2 = 0.0f;
  float current_l3 = 0.0f;
  float active_power_plus = 0.0f;
  float active_power_minus = 0.0f;
  float active_energy_plus = 0.0f;
  float active_energy_minus = 0.0f;
  float reactive_energy_plus = 0.0f;
  float reactive_energy_minus = 0.0f;
  float power_factor = 0.0f;
  char timestamp[27]{};
  char meternumber[13]{};
};

class DlmsMeterComponent : public Component, public uart::UARTDevice {
 public:
  DlmsMeterComponent() = default;

  void setup() override;
  void dump_config() override;
  void loop() override;

  void set_decryption_key(const char *hex_key);

  void publish_sensors(MeterData &data) {
#define DLMS_METER_PUBLISH_SENSOR(s) \
  if (this->s##_sensor_ != nullptr) \
    this->s##_sensor_->publish_state(data.s);
    DLMS_METER_SENSOR_LIST(DLMS_METER_PUBLISH_SENSOR, )

#define DLMS_METER_PUBLISH_TEXT_SENSOR(s) \
  if (this->s##_text_sensor_ != nullptr) \
    this->s##_text_sensor_->publish_state(data.s);
    DLMS_METER_TEXT_SENSOR_LIST(DLMS_METER_PUBLISH_TEXT_SENSOR, )
  }

  DLMS_METER_SENSOR_LIST(SUB_SENSOR, )
  DLMS_METER_TEXT_SENSOR_LIST(SUB_TEXT_SENSOR, )

 protected:
  void on_data_(const char *obis_code, float float_val, const char *str_val, bool is_numeric);

#if defined(USE_ESP8266_FRAMEWORK_ARDUINO)
  dlms_parser::Aes128GcmDecryptorBearSsl decryptor_;
#elif defined(USE_ESP32)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
  dlms_parser::Aes128GcmDecryptorTfPsa decryptor_;
#else
  dlms_parser::Aes128GcmDecryptorMbedTls decryptor_;
#endif
#endif
  dlms_parser::DlmsParser parser_{this->decryptor_};

  MeterData meter_data_{};
  std::vector<uint8_t> receive_buffer_;
  uint32_t last_read_{0};
  static constexpr uint32_t READ_TIMEOUT_MS = 1000;
  static constexpr size_t MAX_RECEIVE_SIZE = 600;  // HDLC frames can be larger than MBUS
};

}  // namespace esphome::dlms_meter
