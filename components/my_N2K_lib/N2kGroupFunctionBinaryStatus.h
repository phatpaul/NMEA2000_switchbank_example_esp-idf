/*
N2kGroupFunctionBinaryStatus.h

Copyright (c) 2015-2020 Timo Lappalainen, Kave Oy, www.kave.fi

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/*
  The file contains default group function handler classes for PGN 60928, 126993.
  These handlers are mandatory for NMEA 2000 certified devices.
*/

#ifndef _N2kGroupFunctionBinaryStatus_H_
#define _N2kGroupFunctionBinaryStatus_H_

#include "NMEA2000_CompilerDefns.h"
#include "N2kGroupFunction.h"

#if !defined(N2K_NO_GROUP_FUNCTION_SUPPORT)

//*****************************************************************************
// Handles requests and instance changes
class tN2kGroupFunctionHandlerForPGN127501 : public tN2kGroupFunctionHandler {
  public:
    // Function must return, does device have given instance
    typedef bool (*tHasInstanceFn)(uint8_t _Instance);
    // If SetNext is true, function must set next binary instace message from given instance and set _Instance to set instance.
    // If there is not next, function return false and set _Instance to 0xff
    // If _Instance is 0xff, function must return binary status for firts instance.
    typedef bool (*tSetBinaryStatusMessageFn)(uint8_t &_Instance, tN2kMsg &N2kMsg, bool SetNext);
    // Function should change _NewInstance for _OldInstance.
    typedef bool (*tChangeInstanceFn)(uint8_t _OldInstance, uint8_t _NewInstance);
    typedef bool (*tChangeTransmissionInterval)(uint8_t _Instance, uint32_t TransmissionInterval, uint16_t TransmissionIntervalOffset);
    //using tN2kChangeInstanceFn = bool (*)(uint8_t _Instance);
  protected:
    tHasInstanceFn HasInstance;
    tSetBinaryStatusMessageFn SetMessage;
    tChangeInstanceFn ChangeInstance;
    tChangeTransmissionInterval ChangeTransmissionInterval;

    virtual bool HandleRequest(const tN2kMsg &N2kMsg,
                               uint32_t TransmissionInterval,
                               uint16_t TransmissionIntervalOffset,
                               uint8_t  NumberOfParameterPairs,
                               int iDev);
//    virtual bool HandleCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting, uint8_t NumberOfParameterPairs, int iDev);
    virtual bool HandleWriteFields(const tN2kMsg &N2kMsg,
                                  uint16_t ManufacturerCode, // This will be set to 0xffff for non-propprietary PNGs
                                  uint8_t IndustryGroup, // This will be set to 0xff for non-propprietary PNGs
                                  uint8_t UniqueID,
                                  uint8_t NumberOfSelectionPairs,
                                  uint8_t NumberOfParameterPairs,
                                  int iDev);
  public:
    tN2kGroupFunctionHandlerForPGN127501(tNMEA2000 *_pNMEA2000,
      tHasInstanceFn _HasInstance,
      tSetBinaryStatusMessageFn _SetMessage,
      tChangeInstanceFn _ChangeInstance,
      tChangeTransmissionInterval _ChangeTransmissionInterval=0
    );
};

#endif

#endif
