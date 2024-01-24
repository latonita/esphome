#include "ina2xx_i2c.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ina2xx_i2c {

static const char *const TAG = "ina2xx_i2c";

void INA2XXI2C::setup() {
  auto err = this->write(nullptr, 0);
  if (err != i2c::ERROR_OK) {
    this->mark_failed();
    return;
  }
  super::setup();
}

void INA2XXI2C::dump_config() {
  super::dump_config();
  LOG_I2C_DEVICE(this);
}

bool INA2XXI2C::read_ina_register_(uint8_t a_register, uint8_t *data, size_t len) {
  auto ret = this->read_register(a_register, data, len);
  if (ret != i2c::ERROR_OK) {
    ESP_LOGD(TAG, "read_ina_register_ failed. Reg=0x%02X Err=%d", a_register, ret);
  }
  return ret == i2c::ERROR_OK;
}

bool INA2XXI2C::write_ina_register_(uint8_t a_register, const uint8_t *data, size_t len) {
  auto ret = this->write_register(a_register, data, len);
  if (ret != i2c::ERROR_OK) {
    ESP_LOGD(TAG, "write_register failed. Reg=0x%02X Err=%d", a_register, ret);
  }
  return ret == i2c::ERROR_OK;
}
}  // namespace ina2xx_i2c
}  // namespace esphome
