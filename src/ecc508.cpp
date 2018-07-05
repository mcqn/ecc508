#include "ecc508.h"

uint16_t calcCRC16( const unsigned char *data, size_t length )
{
	// THIS NEEDS REPLACING WITH MIT LICENCED EQUIVALENT
        uint8_t counter;
        uint16_t crc_register = 0;
        uint16_t polynom = 0x8005;
        uint8_t shift_register;
        uint8_t data_bit, crc_bit;

        for (counter = 0; counter < length; counter++)
        {
                for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1)
                {
                        data_bit = (data[counter] & shift_register) ? 1 : 0;
                        crc_bit = crc_register >> 15;
                        crc_register <<= 1;
                        if (data_bit != crc_bit)
                                crc_register ^= polynom;
                }
        }

	return crc_register & 0xFFFF;
}


void ecc508::begin()
{
  Wire.begin(); // join i2c bus (address optional for master)
}

int ecc508::sendCommand(uint8_t aCommand, uint8_t aParam1, uint16_t aParam2)
{
  Wire.beginTransmission(kI2CAddress);
  uint8_t packet[kMinCommandLength];
  uint16_t crc;
  packet[0] = kMinCommandLength + sizeof(crc);
  packet[1] = aCommand;
  packet[2] = aParam1;
  packet[3] = aParam2 >> 8;
  packet[4] = aParam2 & 0xff;
  crc = calcCRC16(packet, packet[0]-sizeof(crc));

  Wire.write(kModeCommand);
  Wire.write(packet, packet[0]-sizeof(crc));
  Wire.write((uint8_t*)&crc, sizeof(crc));
  return Wire.endTransmission();
}


int ecc508::getRandom(uint8_t* aBuffer, int aLength)
{
  if (aLength > kMaxResponseSizeRandom)
  {
    return kErrorSize;
  }

  int ret = kErrorWatchdogLooming;
  
  while (ret == kErrorWatchdogLooming)
  {
    int tries = 0;
    do
    {
      ret = sendCommand(kCommandRandom, 0, 0);
      if (ret != 0)
      {
        delay(kSendCommandRetryDelay);
      }
    }
    while ((ret != 0) && (tries++ < 2));

    if (ret == 0)
    {
      // Command sent okay, wait for a reply
      iCurrentResponseTime = kResponseTimeRandom;
      iCurrentTimeout = millis()+kMaxResponseTimeRandom;
      iExpectedResponseSize = kMaxResponseSizeRandom;
      ret = waitForResponse();
      if (ret == kErrorSuccess)
      {
	if (iResponseBuffer[0] == kResponseWrapperLength + kMaxResponseSizeRandom)
	{
          // Got a correct response
	  // Now iResponseBuffer will contain the result
	  // Copy the random bytes into the return buffer
	  memcpy(aBuffer, &iResponseBuffer[1], aLength);
	}
	else
	{
	  ret = kErrorSize;
	}
      }
    }
  }
  return ret;
}



int ecc508::getVersion(uint32_t& aVersion)
{
  int ret = kErrorWatchdogLooming;
  
  while (ret == kErrorWatchdogLooming)
  {
    int tries = 0;
    do
    {
      ret = sendCommand(kCommandInfo, 0, 0);
      if (ret != 0)
      {
        delay(kSendCommandRetryDelay);
      }
    }
    while ((ret != 0) && (tries++ < 2));

    if (ret == 0)
    {
      // Command sent okay, wait for a reply
      iCurrentResponseTime = kResponseTimeInfo;
      iCurrentTimeout = millis()+kMaxResponseTimeInfo;
      iExpectedResponseSize = kMaxResponseSizeInfo;
      ret = waitForResponse();
      if (ret == kErrorSuccess)
      {
	if (iResponseBuffer[0] == kResponseWrapperLength + kMaxResponseSizeInfo)
	{
          // Got a correct response
	  // Now iResponseBuffer will contain the result
          aVersion = (iResponseBuffer[1] << 24) | (iResponseBuffer[2] << 16)
		  | (iResponseBuffer[3] << 8) | (iResponseBuffer[4]);
	}
	else
	{
	  ret = kErrorSize;
	}
      }
    }
  }
  return ret;
}

int ecc508::waitForResponse()
{
  // Give the command time to run
  delay(iCurrentResponseTime);
  // Now poll for a reply
  int ret = 0;
  while ((ret == 0) && (millis() <= iCurrentTimeout))
  {
    ret = Wire.requestFrom(kI2CAddress, iExpectedResponseSize + kResponseWrapperLength);
    delay(kPollInterval);
  }
  if (ret != 0)
  {
    // We've some sort of response read it in
    int idx = 0;
    while (Wire.available() && (idx < kResponseBufferSize))
    {
      iResponseBuffer[idx++] = Wire.read();
    }
    // check length, make sure we've got at least as many bytes as the packet length expects
    if (idx >= iResponseBuffer[0])
    {
      // check CRC
      uint16_t crc = calcCRC16(iResponseBuffer, iResponseBuffer[0]-sizeof(crc));
      if (crc != (iResponseBuffer[iResponseBuffer[0]-sizeof(crc)] | iResponseBuffer[iResponseBuffer[0]-sizeof(crc)+1] << 8) )
      {
        return kErrorCRC;
      }
      // See if it's an error packet
      if (iResponseBuffer[0] == 4)
      {
        // Only one byte of data, and it'll be a status code - either 0 for success or an error
	// Either way, it'll be what we want to return as the result code
	return iResponseBuffer[1];
      }
      else
      {
        // Otherwise the data is command-specific, but was received okay
	return kErrorSuccess;
      }
    }
    else
    {
      return kErrorSize;
    }
  }
  else
  {
    // We've timed out
    return kErrorTimeout;
  }
}


