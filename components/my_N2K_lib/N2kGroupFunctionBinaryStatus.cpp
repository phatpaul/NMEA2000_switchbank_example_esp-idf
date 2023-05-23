/*
N2kGroupFunctionBinaryStatus.cpp

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

#include <string.h>
#include "N2kGroupFunctionBinaryStatus.h"
#include "NMEA2000.h"

#if !defined(N2K_NO_GROUP_FUNCTION_SUPPORT)

// *****************************************************************************
tN2kGroupFunctionHandlerForPGN127501::tN2kGroupFunctionHandlerForPGN127501(tNMEA2000 *_pNMEA2000,
     tHasInstanceFn _HasInstance,
     tSetBinaryStatusMessageFn _SetMessage,
     tChangeInstanceFn _ChangeInstance,
     tChangeTransmissionInterval _ChangeTransmissionInterval) :
  tN2kGroupFunctionHandler(_pNMEA2000,127501L),
  HasInstance(_HasInstance), SetMessage(_SetMessage), ChangeInstance(_ChangeInstance),
  ChangeTransmissionInterval(_ChangeTransmissionInterval) {
}

#define N2kPGN127501_Instance_field 1

// *****************************************************************************
// See document http://www.nmea.org/Assets/20140710%20nmea-2000-060928%20iso%20address%20claim%20pgn%20corrigendum.pdf
// For requirements for handling Group function request for PGN 60928
bool tN2kGroupFunctionHandlerForPGN127501::HandleRequest(const tN2kMsg &N2kMsg,
                               uint32_t TransmissionInterval,
                               uint16_t TransmissionIntervalOffset,
                               uint8_t  NumberOfParameterPairs,
                               int iDev) {
  tN2kGroupFunctionTransmissionOrPriorityErrorCode pec=GetRequestGroupFunctionTransmissionOrPriorityErrorCode(TransmissionInterval,TransmissionIntervalOffset);
  bool MatchFilter=true;
  tN2kMsg N2kRMsg;
  uint8_t Instance=0xff;

  // Start to build response
  SetStartAcknowledge(N2kRMsg,N2kMsg.Source,PGN,
                      N2kgfPGNec_Acknowledge,  // What we actually should response as PGN error, if we have invalid field?
                      pec,
                      NumberOfParameterPairs);
  N2kRMsg.Destination=N2kMsg.Source;

  if ( NumberOfParameterPairs>0 ) { // We need to filter accroding to fiels
    int i;
    int Index;
    uint8_t field;
    tN2kGroupFunctionParameterErrorCode FieldErrorCode;
    bool FoundInvalidField=false;

    StartParseRequestPairParameters(N2kMsg,Index);
    // Next read new field values. Note that if message is not broadcast, we need to parse all fields always.
    for (i=0; i<NumberOfParameterPairs && (MatchFilter || !tNMEA2000::IsBroadcast(N2kMsg.Destination)); i++) {
      if ( !FoundInvalidField) {
        field=N2kMsg.GetByte(Index);
        switch (field) {
          case N2kPGN127501_Instance_field:
            if ( Instance==0xff && HasInstance!=0 && HasInstance(Instance=N2kMsg.GetByte(Index)) ) { // Matches to our device
              MatchRequestField(Instance,Instance,(uint8_t)0xff,MatchFilter,FieldErrorCode);
            } else {
              MatchFilter=false;
              FieldErrorCode=N2kgfpec_RequestOrCommandParameterOutOfRange;
            }
            break;
          default:
            FieldErrorCode=N2kgfpec_InvalidRequestOrCommandParameterField;
            MatchFilter=false;
            FoundInvalidField=true;
        }
      } else {
        // If there is any invalid field, we can not parse others, since we do not
        // know right data length. So fo rest of the fields we can only send response below.
        FieldErrorCode=N2kgfpec_TemporarilyUnableToComply;
      }
      AddAcknowledgeParameter(N2kRMsg,i,FieldErrorCode);
    }

  }

  pec=( (ChangeTransmissionInterval!=0 && ChangeTransmissionInterval(Instance,TransmissionInterval,0))?N2kgfTPec_Acknowledge:pec );

  // Send Acknowledge, if request was not broadcast and it did not match
  if ( (!MatchFilter || pec!=N2kgfTPec_Acknowledge || SetMessage==0) && !tNMEA2000::IsBroadcast(N2kMsg.Destination)) {
    // If filter matched, but we do not have set function, report error.
    if ( MatchFilter && SetMessage==0 ) ChangePNGErrorCode(N2kRMsg,N2kgfPGNec_PGNTemporarilyNotAvailable);
    pNMEA2000->SendMsg(N2kRMsg,iDev);
  }

  if ( MatchFilter ) {
    tN2kMsg N2kRequestedMsg;
    bool SetNext=(Instance==0xff); // Loop all, if Instance has not been defined
    uint8_t SendInstance=Instance;

    // Call once first. SendInstance should be the requested or first instance.
    if ( SetMessage!=0 ) {
      bool SetOK;
      do {
        SetOK=SetMessage(SendInstance,N2kRequestedMsg,SetNext);
        if ( SetOK ) {
          N2kRequestedMsg.Destination=N2kMsg.Source;
          if ( tNMEA2000::IsBroadcast(N2kMsg.Destination) ) N2kRequestedMsg.Destination=N2kMsg.Destination;
          pNMEA2000->SendMsg(N2kRequestedMsg,iDev);
        }
      } while ( SetOK && SendInstance!=Instance );
    }
  }

  return true;
}

/*
// *****************************************************************************
// Command group function for 127501 can be used to set instance field
bool tN2kGroupFunctionHandlerForPGN127501::HandleCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting, uint8_t NumberOfParameterPairs, int iDev) {
  int i;
  int Index;
  uint8_t field;
  uint8_t NewInstance;
  tN2kGroupFunctionTransmissionOrPriorityErrorCode pec=N2kgfTPec_Acknowledge;
  tN2kGroupFunctionParameterErrorCode PARec;
  tN2kMsg N2kRMsg;

 		if (PrioritySetting != 0x08 || PrioritySetting != 0x0f || PrioritySetting != 0x09) pec = N2kgfTPec_TransmitIntervalOrPriorityNotSupported;

    SetStartAcknowledge(N2kRMsg,N2kMsg.Source,PGN,
                        N2kgfPGNec_Acknowledge,  // What we actually should response as PGN error, if we have invalid field?
                        pec,
                        NumberOfParameterPairs);

    StartParseCommandPairParameters(N2kMsg,Index);
    // Next read new field values
    for (i=0; i<NumberOfParameterPairs; i++) {
      field=N2kMsg.GetByte(Index);
      PARec=N2kgfpec_Acknowledge;
      switch (field) {
        case N2kPGN127501_Instance_field:
          NewInstance=N2kMsg.GetByte(Index);
          if ( ChangeInstance==0 || !ChangeInstance(0xff,NewInstance) ) {
            PARec=N2kgfpec_InvalidRequestOrCommandParameterField;
          }
          break;
        default:
          PARec=N2kgfpec_InvalidRequestOrCommandParameterField;
      }
      AddAcknowledgeParameter(N2kRMsg,i,PARec);
    }

    pNMEA2000->SendMsg(N2kRMsg,iDev);

    return true;
}
*/

// *****************************************************************************
// HandleWriteFields function for 127501 can be used to set instance field
bool tN2kGroupFunctionHandlerForPGN127501::HandleWriteFields(const tN2kMsg &N2kMsg,
                              uint16_t ManufacturerCode, // This will be set to 0xffff for non-propprietary PNGs
                              uint8_t IndustryGroup, // This will be set to 0xff for non-propprietary PNGs
                              uint8_t UniqueID,
                              uint8_t NumberOfSelectionPairs,
                              uint8_t NumberOfParameterPairs,
                              int iDev) {
  bool MatchFilter=true;
  tN2kMsg N2kAckMsg,N2kReplyMsg;
  uint8_t Instance=0xff;

  SetStartAcknowledge(N2kAckMsg,N2kMsg.Source,PGN,
                      N2kgfPGNec_Acknowledge,
                      N2kgfTPec_Acknowledge,
                      NumberOfSelectionPairs);
  SetStartWriteReply(N2kReplyMsg,N2kMsg.Source,PGN,
                      ManufacturerCode,
                      IndustryGroup,
                      UniqueID,
                      NumberOfSelectionPairs,
                      NumberOfParameterPairs,
                      Proprietary
                    );

  int Index;
  StartParseReadOrWriteParameters(N2kMsg,Proprietary,Index);

  if ( NumberOfSelectionPairs>0 ) { // We need to filter accroding to fields
    int i;
    uint8_t field;
    tN2kGroupFunctionParameterErrorCode FieldErrorCode;
    bool FoundInvalidField=false;

    for (i=0; i<NumberOfSelectionPairs; i++) {
      if ( !FoundInvalidField) {
        field=N2kMsg.GetByte(Index);
        N2kReplyMsg.AddByte(field);
        switch (field) {
          case N2kPGN127501_Instance_field:
            if ( Instance==0xff && HasInstance!=0 && HasInstance(Instance=N2kMsg.GetByte(Index)) ) { // Matches to our device
              MatchRequestField(Instance,Instance,(uint8_t)0xff,MatchFilter,FieldErrorCode);
            } else {
              MatchFilter=false;
              FieldErrorCode=N2kgfpec_RequestOrCommandParameterOutOfRange;
            }
            N2kReplyMsg.AddByte(Instance);
            break;
          default:
            if ( field>0 && field<30 ) { // Right field, but we do not accept it
              N2kReplyMsg.AddByte(0xff);
            } else {
              FoundInvalidField=true;
            }
            FieldErrorCode=N2kgfpec_InvalidRequestOrCommandParameterField;
            MatchFilter=false;
        }
      } else {
        // If there is any invalid field, we can not parse others, since we do not
        // know right data length. So fo rest of the fields we can only send response below.
        FieldErrorCode=N2kgfpec_TemporarilyUnableToComply;
      }
      AddAcknowledgeParameter(N2kAckMsg,i,FieldErrorCode);
    }
  }

  if ( !MatchFilter || Instance==0xff ) {
    pNMEA2000->SendMsg(N2kAckMsg,iDev);
  } else {
    // Start build acknowledge again
    SetStartAcknowledge(N2kAckMsg,N2kMsg.Source,PGN,
                        N2kgfPGNec_Acknowledge,
                        N2kgfTPec_Acknowledge,
                        NumberOfParameterPairs);

    if ( NumberOfParameterPairs>0 ) {
      int i;
      uint8_t field;
      tN2kGroupFunctionParameterErrorCode FieldErrorCode;
      bool FoundInvalidField=false;

      for (i=0; i<NumberOfParameterPairs; i++) {
        if ( !FoundInvalidField) {
          field=N2kMsg.GetByte(Index);
          N2kReplyMsg.AddByte(field);
          switch (field) {
            case N2kPGN127501_Instance_field:
              uint8_t NewInstance;
              NewInstance=N2kMsg.GetByte(Index);
              if ( ChangeInstance==0 || !ChangeInstance(Instance,NewInstance) ) NewInstance=Instance; // Could not change. Return old value.
              N2kReplyMsg.AddByte(NewInstance);
              FieldErrorCode=N2kgfpec_Acknowledge;
              break;
            default:
              if ( field>0 && field<30 ) { // Right field, but we do not accept it
                FieldErrorCode=N2kgfpec_ReadOrWriteIsNotSupported;
              } else {
                FoundInvalidField=true;
                FieldErrorCode=N2kgfpec_TemporarilyUnableToComply;
              }
              MatchFilter=false;
          }
        } else {
          // If there is any invalid field, we can not parse others, since we do not
          // know right data length. So fo rest of the fields we can only send response below.
          FieldErrorCode=N2kgfpec_TemporarilyUnableToComply;
        }
        AddAcknowledgeParameter(N2kAckMsg,i,FieldErrorCode);
      }
    }

    if ( MatchFilter ) {
      pNMEA2000->SendMsg(N2kReplyMsg,iDev);
    } else {
      pNMEA2000->SendMsg(N2kAckMsg,iDev);
    }

  }

  return true;
}

#endif
