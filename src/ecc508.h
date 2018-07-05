
#ifndef __ECC508_H__
#define __ECC508_H__

#include <Arduino.h>
#include <Wire.h>

class ecc508
{
public:
  static const uint8_t kI2CAddress =  0x60;
  static const uint8_t kModeCommand = 0x03;
  static const uint8_t kCommandRandom = 0x1b;
  static const uint8_t kCommandInfo = 0x30;
  // How quickly we'll poll for responses, after the initial response time delay (in ms)
  static const int kPollInterval = 2;
  static const int kSendCommandRetryDelay = 1;
  // Typical response times, from the datasheet, table 9.4
  static const int kResponseTimeInfo = 1;
  static const int kResponseTimeRandom = 1;
  // Max response times, from the datasheet, table 9.4
  static const int kMaxResponseTimeInfo = 4;
  static const int kMaxResponseTimeRandom = 23;
  // Max response sizes for each command
  static const int kMaxResponseSizeInfo = 4;
  static const int kMaxResponseSizeRandom = 32;
  static const int kResponseBufferSize = 64+1+2; // Max response size + length + CRC
  // Error values
  static const uint8_t kErrorSuccess = 0x00;
  static const uint8_t kErrorMiscompare = 0x01;
  static const uint8_t kErrorParse = 0x03;
  static const uint8_t kErrorECCFault = 0x05;
  static const uint8_t kErrorExecution = 0x0F;
  static const uint8_t kErrorAfterWake = 0x11;
  static const uint8_t kErrorWatchdogLooming = 0xEE;
  static const uint8_t kErrorCRC = 0xFF;
  static const uint8_t kErrorSize = 0xFD;
  static const uint8_t kErrorTimeout = 0xFE;
  static const uint8_t kMinCommandLength = 1+1+3; // length + command + 3 bytes of params
  static const uint8_t kResponseWrapperLength = 1+2; // length byte + 2 bytes of CRC
public:
  void begin();
  int sendCommand(uint8_t aCommand, uint8_t aParam1, uint16_t aParam2);
  // Get ecc508 chip version
  // Parameters:
  //   aVersion - upon successful return, contains the version number
  // Returns:
  //   kErrorSuccess if successful, otherwise an error code
  int getVersion(uint32_t& aVersion);
  int getRandom(uint8_t* aBuffer, int aLength);
protected:
  int waitForResponse();
protected:
  int iCurrentResponseTime;
  unsigned long iCurrentTimeout;
  int iExpectedResponseSize;
  uint8_t iResponseBuffer[kResponseBufferSize];
};

#endif
