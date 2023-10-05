#ifndef _ESPFC_HARDWARE_H_
#define _ESPFC_HARDWARE_H_

#include <Arduino.h>
#include "Target/Target.h"
#include "Model.h"
#include "Device/BusDevice.h"
#if defined(ESPFC_I2C_0)
#include "Device/BusI2C.h"
#endif
#if defined(ESPFC_SPI_0)
#include "Device/BusSPI.h"
#endif
#include "Device/GyroDevice.h"
#include "Device/GyroMPU6050.h"
#include "Device/GyroMPU6500.h"
#include "Device/GyroMPU6555.h"
#include "Device/GyroMPU9250.h"
#include "Device/GyroLSM6DSO.h"
#include "Device/GyroICM20602.h"
#include "Device/MagHMC5338L.h"
#include "Device/MagAK8963.h"
#include "Device/BaroDevice.h"
#include "Device/BaroBMP085.h"
#include "Device/BaroBMP280.h"

namespace {
#if defined(ESPFC_SPI_0)
#ifdef CONFIG_IDF_TARGET_ESP32S3
  #define VSPI FSPI
#endif
#ifdef ESP32
  static SPIClass SPI1(VSPI);
#endif
  static Espfc::Device::BusSPI spiBus(ESPFC_SPI_0_DEV);
#endif
#if defined(ESPFC_I2C_0)
  static Espfc::Device::BusI2C i2cBus(WireInstance);
#endif
  static Espfc::Device::GyroMPU6050 mpu6050;
  static Espfc::Device::GyroMPU6500 mpu6500;
  static Espfc::Device::GyroMPU6555 mpu6555;
  static Espfc::Device::GyroMPU9250 mpu9250;
  static Espfc::Device::GyroLSM6DSO lsm6dso;
  static Espfc::Device::GyroICM20602 icm20602;
  static Espfc::Device::MagHMC5338L hmc5883l;
  static Espfc::Device::MagAK8963 ak8963;
  static Espfc::Device::BaroBMP085 bmp085;
  static Espfc::Device::BaroBMP280 bmp280;
}

namespace Espfc {

class Hardware
{
  public:
    Hardware(Model& model): _model(model) {}

    int begin()
    {
      initBus();
      detectGyro();
      detectMag();
      detectBaro();
      return 1;
    }

    void onI2CError()
    {
      _model.state.i2cErrorCount++;
      _model.state.i2cErrorDelta++;
    }

    void initBus()
    {
#if defined(ESPFC_SPI_0)
      int spiResult = spiBus.begin(_model.config.pin[PIN_SPI_0_SCK], _model.config.pin[PIN_SPI_0_MOSI], _model.config.pin[PIN_SPI_0_MISO]);
      _model.logger.info().log(F("SPI SETUP")).log(_model.config.pin[PIN_SPI_0_SCK]).log(_model.config.pin[PIN_SPI_0_MOSI]).log(_model.config.pin[PIN_SPI_0_MISO]).logln(spiResult);
#endif
#if defined(ESPFC_I2C_0)
      int i2cResult = i2cBus.begin(_model.config.pin[PIN_I2C_0_SDA], _model.config.pin[PIN_I2C_0_SCL], _model.config.i2cSpeed * 1000ul);
      i2cBus.onError = std::bind(&Hardware::onI2CError, this);
      _model.logger.info().log(F("I2C SETUP")).log(_model.config.pin[PIN_I2C_0_SDA]).log(_model.config.pin[PIN_I2C_0_SCL]).log(_model.config.i2cSpeed).logln(i2cResult);
#endif
    }

    void detectGyro()
    {
      if(_model.config.gyroDev == GYRO_NONE) return;

      Espfc::Device::GyroDevice * detectedGyro = nullptr;
#if defined(ESPFC_SPI_0)
      if(_model.config.pin[PIN_SPI_CS0] != -1)
      {
        digitalWrite(_model.config.pin[PIN_SPI_CS0], HIGH);
        pinMode(_model.config.pin[PIN_SPI_CS0], OUTPUT);
        if(!detectedGyro && detectDevice(mpu9250, spiBus, _model.config.pin[PIN_SPI_CS0])) detectedGyro = &mpu9250;
        if(!detectedGyro && detectDevice(mpu6500, spiBus, _model.config.pin[PIN_SPI_CS0])) detectedGyro = &mpu6500;
        if(!detectedGyro && detectDevice(mpu6555, spiBus, _model.config.pin[PIN_SPI_CS0])) detectedGyro = &mpu6555;
        if(!detectedGyro && detectDevice(icm20602, spiBus, _model.config.pin[PIN_SPI_CS0])) detectedGyro = &icm20602;
        if(!detectedGyro && detectDevice(lsm6dso, spiBus, _model.config.pin[PIN_SPI_CS0])) detectedGyro = &lsm6dso;
      }
#endif
#if defined(ESPFC_I2C_0)
      if(_model.config.pin[PIN_I2C_0_SDA] != -1 && _model.config.pin[PIN_I2C_0_SCL] != -1)
      {
        if(!detectedGyro && detectDevice(mpu9250, i2cBus)) detectedGyro = &mpu9250;
        if(!detectedGyro && detectDevice(mpu6500, i2cBus)) detectedGyro = &mpu6500;
        if(!detectedGyro && detectDevice(mpu6555, i2cBus)) detectedGyro = &mpu6555;
        if(!detectedGyro && detectDevice(icm20602, i2cBus)) detectedGyro = &icm20602;
        if(!detectedGyro && detectDevice(mpu6050, i2cBus)) detectedGyro = &mpu6050;
        if(!detectedGyro && detectDevice(lsm6dso, i2cBus)) detectedGyro = &lsm6dso;
      }
#endif
      if(!detectedGyro) return;

      detectedGyro->setDLPFMode(_model.config.gyroDlpf);
      _model.state.gyroDev = detectedGyro;
      _model.state.gyroPresent = (bool)detectedGyro;
      _model.state.accelPresent = _model.state.gyroPresent && _model.config.accelDev != GYRO_NONE;
      _model.state.gyroClock = detectedGyro->getRate();
    }

    void detectMag()
    {
      if(_model.config.magDev == MAG_NONE) return;

      Espfc::Device::MagDevice * detectedMag  = nullptr;
#if defined(ESPFC_SPI_0)
      if(_model.config.pin[PIN_SPI_CS0] != -1 && _model.state.gyroDev && _model.state.gyroDev->getType() == GYRO_MPU9250)
      {
        if(!detectedMag && detectDevice(ak8963, spiBus, _model.config.pin[PIN_SPI_CS0])) detectedMag = &ak8963;
      }
#endif
#if defined(ESPFC_I2C_0)
      if(_model.config.pin[PIN_I2C_0_SDA] != -1 && _model.config.pin[PIN_I2C_0_SCL] != -1)
      {
        if(_model.state.gyroDev && _model.state.gyroDev->getType() == GYRO_MPU9250)
        {
          if(!detectedMag && detectDevice(ak8963, i2cBus)) detectedMag = &ak8963;
        }
        if(!detectedMag && detectDevice(hmc5883l, i2cBus)) detectedMag = &hmc5883l;
      }
#endif
      _model.state.magDev = detectedMag;
      _model.state.magPresent = (bool)detectedMag;
      _model.state.magRate = detectedMag ? detectedMag->getRate() : 0;
    }

    void detectBaro()
    {
      if(_model.config.baroDev == BARO_NONE) return;

      Espfc::Device::BaroDevice * detectedBaro = nullptr;
#if defined(ESPFC_SPI_0)
      if(_model.config.pin[PIN_SPI_CS1] != -1)
      {
        digitalWrite(_model.config.pin[PIN_SPI_CS1], HIGH);
        pinMode(_model.config.pin[PIN_SPI_CS1], OUTPUT);
        if(!detectedBaro && detectDevice(bmp280, spiBus, _model.config.pin[PIN_SPI_CS1])) detectedBaro = &bmp280;
        if(!detectedBaro && detectDevice(bmp085, spiBus, _model.config.pin[PIN_SPI_CS1])) detectedBaro = &bmp085;
      }
#endif
#if defined(ESPFC_I2C_0)
      if(_model.config.pin[PIN_I2C_0_SDA] != -1 && _model.config.pin[PIN_I2C_0_SCL] != -1)
      {
        if(!detectedBaro && detectDevice(bmp280, i2cBus)) detectedBaro = &bmp280;
        if(!detectedBaro && detectDevice(bmp085, i2cBus)) detectedBaro = &bmp085;
      }
#endif
      _model.state.baroDev = detectedBaro;
      _model.state.baroPresent = (bool)detectedBaro;
    }

#if defined(ESPFC_SPI_0)
    template<typename Dev>
    bool detectDevice(Dev& dev, Device::BusSPI& bus, int cs)
    {
      typename Dev::DeviceType type = dev.getType();
      bool status = dev.begin(&bus, cs);
      _model.logger.info().log(F("SPI DETECT")).log(FPSTR(Dev::getName(type))).log(cs).logln(status);
      return status;
    }
#endif

#if defined(ESPFC_I2C_0)
    template<typename Dev>
    bool detectDevice(Dev& dev, Device::BusI2C& bus)
    {
      typename Dev::DeviceType type = dev.getType();
      bool status = dev.begin(&bus);
      _model.logger.info().log(F("I2C DETECT")).log(FPSTR(Dev::getName(type))).logln(status);
      return status;
    }
#endif

    int update()
    {
      return 1;
    }

    static void restart(const Model& model)
    {
      //escMotor.end();
      //if(model.config.output.servoRate) escServo.end();
      targetReset();
    }

  private:
    Model& _model;
};

}

#endif
