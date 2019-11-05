#include<Wire.h>
#include "vl53l1_api.h"

VL53L1_Dev_t                   dev;
VL53L1_DEV                     Dev = &dev;



// Timing budget set through VL53L1_SetMeasurementTimingBudgetMicroSeconds().
#define MEASUREMENT_BUDGET_MS 50

// Interval between measurements, set through
// VL53L1_SetInterMeasurementPeriodMilliSeconds(). According to the API user
// manual (rev 2), "the minimum inter-measurement period must be longer than the
// timing budget + 4 ms." The STM32Cube example from ST uses 500 ms, but we
// reduce this to 55 ms to allow faster readings.
#define INTER_MEASUREMENT_PERIOD_MS 55

int status;
VL53L1_UserRoi_t RoiConfig;

//12x12 because it's a 16x16 array and we're reading it in 4x4 chunks
//int16_t depths[12*12];

//Arduino can't cope, library already uses too much memory
//int16_t depths[6*6];
int16_t depths[4*4];
  
void setup()
{
  uint8_t byteData;
  uint16_t wordData;

  Wire.begin();
  Wire.setClock(400000);
  Serial.begin(115200);

  // This is the default 8-bit slave address (including R/W as the least
  // significant bit) as expected by the API. Note that the Arduino Wire library
  // uses a 7-bit address without the R/W bit instead (0x29 or 0b0101001).
  Dev->I2cDevAddr = 0x52;

  VL53L1_software_reset(Dev);

//  VL53L1_RdByte(Dev, 0x010F, &byteData);
//  Serial.print(F("VL53L1X Model_ID: "));
//  Serial.println(byteData, HEX);
//  VL53L1_RdByte(Dev, 0x0110, &byteData);
//  Serial.print(F("VL53L1X Module_Type: "));
//  Serial.println(byteData, HEX);
//  VL53L1_RdWord(Dev, 0x010F, &wordData);
//  Serial.print(F("VL53L1X: "));
//  Serial.println(wordData, HEX);

  status = VL53L1_WaitDeviceBooted(Dev);
  status = VL53L1_DataInit(Dev);
  status = VL53L1_StaticInit(Dev);
  status = VL53L1_SetDistanceMode(Dev, VL53L1_DISTANCEMODE_SHORT);
  status = VL53L1_SetMeasurementTimingBudgetMicroSeconds(Dev, MEASUREMENT_BUDGET_MS * 1000);
  status = VL53L1_SetInterMeasurementPeriodMilliSeconds(Dev, INTER_MEASUREMENT_PERIOD_MS);

  RoiConfig.TopLeftX = 0;
  RoiConfig.TopLeftY = 0;
  RoiConfig.BotRightX = RoiConfig.TopLeftX + 3;
  RoiConfig.BotRightY = RoiConfig.BotRightY + 3;
  status = VL53L1_SetUserROI(Dev, &RoiConfig);

}

void loop() {
  static uint16_t startMs = millis();
  uint8_t isReady;

  // non-blocking check for data ready
  status = VL53L1_GetMeasurementDataReady(Dev, &isReady);

  if(!status)
  {
    if(isReady) //Data ready
    {
//      Serial.print(RoiConfig.TopLeftX);
//      Serial.print(',');
//      Serial.println(RoiConfig.TopLeftY);
      
      static VL53L1_RangingMeasurementData_t RangingData;
      status = VL53L1_GetRangingMeasurementData(Dev, &RangingData);
      if(!status)
      {
        if(RangingData.RangeStatus == 0){
          //Serial.print(RoiConfig.TopLeftX/4 + ((RoiConfig.TopLeftY/4) * 4));
          //Serial.print(",");
          //Serial.println(RangingData.RangeMilliMeter);
          
          depths[RoiConfig.TopLeftX/4 + ((RoiConfig.TopLeftY/4) * 4)] = RangingData.RangeMilliMeter;
        }
        else
        {
          //Serial.print(RoiConfig.TopLeftX/4 + ((RoiConfig.TopLeftY/4) * 4));
          //Serial.println(", BAD");
          
          depths[RoiConfig.TopLeftX/4 + ((RoiConfig.TopLeftY/4) * 4)] = 0;
        }
      }
      
      //Update the ROI
      RoiConfig.TopLeftX += 4;
      if(RoiConfig.TopLeftX > 12)
      {
        //End of row, roll to next row
        RoiConfig.TopLeftX = 0;
        RoiConfig.TopLeftY += 4;
        if(RoiConfig.TopLeftY > 12)
        {
          //End of device, reset to start
          RoiConfig.TopLeftY = 0;  
          //And dump the range data
          dump_depths();  
        }
      }
      //4x4 is the minimum ROI
      RoiConfig.BotRightX = RoiConfig.TopLeftX + 3;
      RoiConfig.BotRightY = RoiConfig.BotRightY + 3;

      status = VL53L1_SetUserROI(Dev, &RoiConfig);
            
      // Range again
      VL53L1_ClearInterruptAndStartMeasurement(Dev);
      startMs = millis();
    }
    else if((uint16_t)(millis() - startMs) > VL53L1_RANGE_COMPLETION_POLLING_TIMEOUT_MS)
    {
      //Serial.println(F("Timeout waiting for data ready."));
      VL53L1_ClearInterruptAndStartMeasurement(Dev);
      startMs = millis();
    }
  }
  else
  {
    Serial.print(F("Error getting data ready: "));
    Serial.println(status);    
  }
}

void dump_depths()
{
  Serial.print(depths[0]);
  for(int idx=1; idx < (4*4); idx++)
  {
    Serial.print(",");
    Serial.print(depths[idx]);
  }
  Serial.println(" ");
}
