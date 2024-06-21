#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include "iqs7222c_defines.h"
#include "iqs7222c_registers.h"

#include <vector>

namespace esphome {
namespace iqs7222c {

struct IQS7222CStore {
  volatile bool touched{true};
  volatile bool iqs7222c_deviceRDY{true};
  bool init{false};
  ISRInternalGPIOPin irq_pin;
  static void gpio_intr(IQS7222CStore *store);
};

// class IQS7222CChannel : public binary_sensor::BinarySensor {
//  public:
//   void set_channel(uint8_t channel) { channel_ = channel; }
//   void process(uint8_t data) { this->publish_state(static_cast<bool>(data & (1 << this->channel_))); }

//  protected:
//   uint8_t channel_{0};
// };

class IQS7222CComponent : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void loop() override;

  void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }
  // void set_mclr_pin(GPIOPin *mclr_pin) { this->mclr_pin_ = mclr_pin; }

  // void register_channel(IQS7222CChannel *channel) { this->channels_.push_back(channel); }
  // void set_touch_threshold(uint8_t touch_threshold) { this->touch_threshold_ = touch_threshold; };
  // void set_allow_multiple_touches(bool allow_multiple_touches) {
  //   this->allow_multiple_touches_ = allow_multiple_touches ? 0x41 : 0x80;
  // };

 protected:
  // bool iqs7222c_deviceRDY{false};
  InternalGPIOPin *interrupt_pin_{};
  void attach_interrupt_(InternalGPIOPin *irq_pin, esphome::gpio::InterruptType type);
  IQS7222CStore store_;

  // GPIOPin *mclr_pin_{nullptr};

  // Public Device States
  iqs7222c_s iqs7222c_state;

  // Public Variables
  IQS7222C_MEMORY_MAP IQSMemoryMap;
  bool new_data_available;

  bool init(void);
  void run(void);

  void queueValueUpdates(void);
  bool readATIactive(void);
  uint16_t getProductNum(bool stopOrRestart);
  uint8_t getmajorVersion(bool stopOrRestart);
  uint8_t getminorVersion(bool stopOrRestart);
  void acknowledgeReset(bool stopOrRestart);
  void ReATI(bool stopOrRestart);
  void SW_Reset(bool stopOrRestart);
  void writeMM(bool stopOrRestart);
  void clearRDY(void);
  bool getRDYStatus(void);

  void setStreamMode(bool stopOrRestart);
  void setEventMode(bool stopOrRestart);
  void setStreamInTouchMode(bool stopOrRestart);

  void updateInfoFlags(bool stopOrRestart);
  iqs7222c_power_modes get_PowerMode(void);
  bool checkReset(void);

  bool channel_touchState(iqs7222c_channel_e channel);
  bool channel_proxState(iqs7222c_channel_e channel);
  uint16_t sliderCoordinate(iqs7222c_slider_e slider);

  void force_I2C_communication(void);

 private:
  // Private Variables
  uint8_t _deviceAddress;

  // Private Methods
  void readRandomBytes(uint8_t memoryAddress, uint8_t numBytes, uint8_t bytesArray[], bool stopOrRestart);
  void writeRandomBytes(uint8_t memoryAddress, uint8_t numBytes, uint8_t bytesArray[], bool stopOrRestart);
  void writeRandomBytes16(uint16_t memoryAddress, uint8_t numBytes, uint8_t bytesArray[], bool stopOrRestart);
  bool getBit(uint8_t data, uint8_t bit_number);
  uint8_t setBit(uint8_t data, uint8_t bit_number);
  uint8_t clearBit(uint8_t data, uint8_t bit_number);

  // std::vector<IQS7222CChannel *> channels_{};
  // uint8_t touch_threshold_{0x20};
  // uint8_t allow_multiple_touches_{0x80};

  // uint8_t iqs7222c_product_id_{0};
  // uint8_t iqs7222c_manufacture_id_{0};
  // uint8_t iqs7222c_revision_{0};

  enum ErrorCode {
    NONE = 0,
    COMMUNICATION_FAILED,
  } error_code_{NONE};
};

}  // namespace iqs7222c
}  // namespace esphome
