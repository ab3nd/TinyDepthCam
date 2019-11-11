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

// 16 configurations, describing each of the ROIs for sensing
VL53L1_UserRoi_t ROIConfigs[16];
// 16 results, one for each ROI
int16_t depths[16];

//For holding the ranging data
static VL53L1_RangingMeasurementData_t RangingData;
  
void setup()
{
  Wire.begin();
  Wire.setClock(400000);
  Serial.begin(115200);

  // This is the default 8-bit slave address (including R/W as the least
  // significant bit) as expected by the API. Note that the Arduino Wire library
  // uses a 7-bit address without the R/W bit instead (0x29 or 0b0101001).
  Dev->I2cDevAddr = 0x52;

  VL53L1_software_reset(Dev);

  status = VL53L1_WaitDeviceBooted(Dev);
  status = VL53L1_DataInit(Dev);
  status = VL53L1_StaticInit(Dev);

  //Slower ranging mode, I want that 100Hz/1.3m range sweetness
  //status = VL53L1_SetDistanceMode(Dev, VL53L1_DISTANCEMODE_SHORT);
  //status = VL53L1_SetMeasurementTimingBudgetMicroSeconds(Dev, MEASUREMENT_BUDGET_MS * 1000);
  //status = VL53L1_SetInterMeasurementPeriodMilliSeconds(Dev, INTER_MEASUREMENT_PERIOD_MS);

  //Fast mode needs these options in this order according to app note AN5263 from ST
  //https://www.st.com/content/ccc/resource/technical/document/application_note/group1/13/fd/27/76/de/e8/46/4d/DM00566701/files/DM00566701.pdf/jcr:content/translations/en.DM00566701.pdf  
  status = VL53L1_SetPresetMode(Dev, VL53L1_PRESETMODE_LITE_RANGING);     
  status = VL53L1_SetDistanceMode(Dev, VL53L1_DISTANCEMODE_SHORT);     
  status = VL53L1_SetMeasurementTimingBudgetMicroSeconds(Dev, 10000);     

  //First measurement is apparently bad, get it and throw it away
  status = VL53L1_StartMeasurement(Dev);
  status = VL53L1_WaitMeasurementDataReady(Dev);
  status = VL53L1_GetRangingMeasurementData(Dev,&RangingData);
  status = VL53L1_ClearInterruptAndStartMeasurement(Dev);
  
  //Set up 16 ROIs
  for(int y = 0; y < 4; y++){
    for(int x = 0; x < 4; x++){
      //An ROI is a top left x, top left y, bottom left x, bottom left y
      ROIConfigs[(x*4) + y] = {(x*4), 15-(y*4), (x*4) + 3, (15-(y*4)) - 3};
    }
  }
}

void loop() {
  for (int ii = 0; ii < 16; ii++) {
    // Blocks here until we have the measurement
    status = VL53L1_WaitMeasurementDataReady(Dev);
    if (!status){
      status = VL53L1_GetRangingMeasurementData(Dev, &RangingData);
      if (status == 0) 
      {
        //Got a valid distance
        depths[ii] = RangingData.RangeMilliMeter;
      }    
    }
    // Set up the ROI
    status = VL53L1_SetUserROI(Dev, &ROIConfigs[ii]);
    
    //VL53L1_clear_interrupt_and_enable_next_range(Dev, VL53L1_DEVICEMEASUREMENTMODE_SINGLESHOT);
    VL53L1_ClearInterruptAndStartMeasurement(Dev);

  }
  dump_depths();
}

void dump_depths()
{
  Serial.print(depths[0]);
  for(int idx=1; idx < 16; idx++)
  {
    Serial.print(",");
    Serial.print(depths[idx]);
  }
  Serial.println(" ");
}
