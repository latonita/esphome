#include "dlms_meter.h"

#include <cstdio>

namespace esphome::dlms_meter {

static constexpr const char *TAG = "dlms_meter";

static void log_callback(dlms_parser::LogLevel level, const char *fmt, va_list args) {
  static char buf[256];
  vsnprintf(buf, sizeof(buf), fmt, args);
  switch (level) {
    case dlms_parser::LogLevel::ERROR:
      ESP_LOGE(TAG, "%s", buf);
      break;
    case dlms_parser::LogLevel::WARNING:
      ESP_LOGW(TAG, "%s", buf);
      break;
    case dlms_parser::LogLevel::INFO:
      ESP_LOGI(TAG, "%s", buf);
      break;
    case dlms_parser::LogLevel::VERBOSE:
      ESP_LOGV(TAG, "%s", buf);
      break;
    default:
      ESP_LOGVV(TAG, "%s", buf);
      break;
  }
}

void DlmsMeterComponent::setup() {
  dlms_parser::Logger::set_log_function(log_callback);
  this->parser_.load_default_patterns();
}

void DlmsMeterComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "DLMS Meter:");
#define DLMS_METER_LOG_SENSOR(s) LOG_SENSOR("  ", #s, this->s##_sensor_);
  DLMS_METER_SENSOR_LIST(DLMS_METER_LOG_SENSOR, )
#define DLMS_METER_LOG_TEXT_SENSOR(s) LOG_TEXT_SENSOR("  ", #s, this->s##_text_sensor_);
  DLMS_METER_TEXT_SENSOR_LIST(DLMS_METER_LOG_TEXT_SENSOR, )
}

void DlmsMeterComponent::loop() {
  size_t avail = this->available();
  if (avail > 0) {
    size_t remaining = MAX_RECEIVE_SIZE - this->receive_buffer_.size();
    if (remaining == 0) {
      ESP_LOGW(TAG, "Receive buffer full, dropping remaining bytes");
    } else {
      if (avail > remaining)
        avail = remaining;
      uint8_t buf[64];
      while (avail > 0) {
        size_t to_read = std::min(avail, sizeof(buf));
        if (!this->read_array(buf, to_read))
          break;
        this->receive_buffer_.insert(this->receive_buffer_.end(), buf, buf + to_read);
        avail -= to_read;
      }
      this->last_read_ = millis();
    }
  }

  if (!this->receive_buffer_.empty() && millis() - this->last_read_ > READ_TIMEOUT_MS) {
    this->meter_data_ = {};
    auto result = this->parser_.parse({this->receive_buffer_.data(), this->receive_buffer_.size()},
                                      [this](const char *obis, float fval, const char *sval, bool is_num) {
                                        this->on_data_(obis, fval, sval, is_num);
                                      });
    if (result.count > 0) {
      ESP_LOGI(TAG, "Received %zu OBIS values", result.count);
      this->publish_sensors(this->meter_data_);
      this->status_clear_warning();
    } else {
      ESP_LOGW(TAG, "Failed to parse %zu received bytes", this->receive_buffer_.size());
    }
    this->receive_buffer_.clear();
  }
}

void DlmsMeterComponent::set_decryption_key(const char *hex_key) {
  auto key = dlms_parser::Aes128GcmDecryptionKey::from_hex(hex_key);
  if (key) {
    this->parser_.set_decryption_key(*key);
  } else {
    ESP_LOGE(TAG, "Invalid decryption key");
  }
}

void DlmsMeterComponent::on_data_(const char *obis_code, float float_val, const char *str_val, bool is_numeric) {
  ESP_LOGV(TAG, "OBIS %s: %s value=%.2f str=%s", obis_code, is_numeric ? "num" : "str", float_val,
           str_val != nullptr ? str_val : "(null)");

  if (is_numeric) {
    struct ObisFloat {
      const char *obis;
      float MeterData::*field;
    };
    static constexpr size_t NUM_FLOAT_MAPPINGS = 13;
    static const ObisFloat FLOAT_MAPPINGS[NUM_FLOAT_MAPPINGS] = {
        {"1.0.32.7.0.255", &MeterData::voltage_l1},          {"1.0.52.7.0.255", &MeterData::voltage_l2},
        {"1.0.72.7.0.255", &MeterData::voltage_l3},          {"1.0.31.7.0.255", &MeterData::current_l1},
        {"1.0.51.7.0.255", &MeterData::current_l2},          {"1.0.71.7.0.255", &MeterData::current_l3},
        {"1.0.1.7.0.255", &MeterData::active_power_plus},    {"1.0.2.7.0.255", &MeterData::active_power_minus},
        {"1.0.1.8.0.255", &MeterData::active_energy_plus},   {"1.0.2.8.0.255", &MeterData::active_energy_minus},
        {"1.0.3.8.0.255", &MeterData::reactive_energy_plus}, {"1.0.4.8.0.255", &MeterData::reactive_energy_minus},
        {"1.0.13.7.0.255", &MeterData::power_factor},
    };
    for (const auto &m : FLOAT_MAPPINGS) {
      if (strcmp(obis_code, m.obis) == 0) {
        this->meter_data_.*m.field = float_val;
        return;
      }
    }
  } else if (str_val != nullptr) {
    if (strcmp(obis_code, "0.0.1.0.0.255") == 0) {
      snprintf(this->meter_data_.timestamp, sizeof(this->meter_data_.timestamp), "%s", str_val);
    } else if (strcmp(obis_code, "0.0.96.1.0.255") == 0) {
      snprintf(this->meter_data_.meternumber, sizeof(this->meter_data_.meternumber), "%s", str_val);
    }
  }
}

}  // namespace esphome::dlms_meter
