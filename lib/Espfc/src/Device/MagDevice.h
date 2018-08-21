#ifndef _ESPFC_DEVICE_MAG_DEVICE_H_
#define _ESPFC_DEVICE_MAG_DEVICE_H_

#include "helper_3dmath.h"
#include "BusAwareDevice.h"

namespace Espfc {

enum MagDeviceType {
  MAG_DEFAULT = 0,
  MAG_NONE    = 1,
  MAG_HMC5883 = 2,
  MAG_AK8975  = 3,
  MAG_AK8963  = 4,
  MAG_MAX
};

namespace Device {

class MagDevice: public BusAwareDevice
{
  public:
    typedef MagDeviceType DeviceType;

    virtual DeviceType getType() const = 0;

    virtual int readMag(VectorInt16& v) = 0;

    virtual void setSampleAveraging(uint8_t averaging) = 0;
    virtual void setSampleRate(uint8_t rate) = 0;
    virtual void setMode(uint8_t mode) = 0;
    virtual void setGain(uint8_t scale) = 0;

    virtual bool testConnection() = 0;

    static const char ** getNames()
    {
      static const char* devChoices[] = { PSTR("AUTO"), PSTR("NONE"), PSTR("HMC5883"), PSTR("AK8975"), PSTR("AK8963"), NULL };
      return devChoices;
    }

    static const char * getName(DeviceType type)
    {
      if(type >= MAG_MAX) return PSTR("?");
      return getNames()[type];
    }
};

}

}

#endif
