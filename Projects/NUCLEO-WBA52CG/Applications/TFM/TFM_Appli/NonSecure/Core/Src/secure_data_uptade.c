/**
  ******************************************************************************
  * @file    secure_data_uptade.c
  * @author  MCD Application Team
  * @brief   Source of secure data update functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include <stdint.h>
#include "app_conf.h"
#include "region_defs.h"
#include "Driver_Flash.h"
#include "secure_data_update.h"

extern ARM_DRIVER_FLASH TFM_Driver_FLASH0;

static const uint32_t magic_trailer_value[] =
{
  0xf395c277,
  0x7fefd260,
  0x0f505235,
  0x8079b62c,
};


int secure_data_update_init(){
  int ret;
  uint32_t sector_address;
  const ARM_FLASH_INFO *data = TFM_Driver_FLASH0.GetInfo();
  
  APP_DBG_MSG("secure_data_update_init: erasing sectors\r\n");
  
  for (sector_address = S_DATA_IMAGE_SECONDARY_PARTITION_OFFSET;
       sector_address < S_DATA_IMAGE_SECONDARY_PARTITION_OFFSET + FLASH_S_DATA_PARTITION_SIZE;
       sector_address += data->sector_size)
  {
    ret = TFM_Driver_FLASH0.EraseSector(sector_address);
    if (ret < 0){
      APP_DBG_MSG("Error with EraseSector: %i at address %x\r\n\n", ret, sector_address);
      return ret;
    }
  }
  return ret;
}

int secure_data_update_write(uint8_t *buf, uint32_t offset, uint32_t buf_sz){
  int ret;
  const ARM_FLASH_INFO *data = TFM_Driver_FLASH0.GetInfo();
  uint32_t program_unit = data->program_unit;
  uint32_t program_offset = 0;
  
  APP_DBG_MSG("secure_data_update_write programmng unit %u\r\n\n", program_unit);
  
  if((buf_sz%program_unit) != 0){
    APP_DBG_MSG("secure_data_update_write buf_sz should be a multiple of %u\r\n\n", program_unit);
    return -1;
  }
  
  for(program_offset=0; program_offset<buf_sz; program_offset+=program_unit){
    
    ret = TFM_Driver_FLASH0.ProgramData(S_DATA_IMAGE_SECONDARY_PARTITION_OFFSET+offset+program_offset, &buf[program_offset], program_unit);
    if (ret != ARM_DRIVER_OK) {
      APP_DBG_MSG("secure_data_update_write failed to write at %x\r\n", S_DATA_IMAGE_SECONDARY_PARTITION_OFFSET+program_offset);
      return -1;
    }
  }
  return 0;
}

int secure_data_udate_install(){
  int ret;
  uint32_t magic_trailer_offset = S_DATA_IMAGE_SECONDARY_PARTITION_OFFSET + (FLASH_S_DATA_PARTITION_SIZE - sizeof(magic_trailer_value));
  
  ret = TFM_Driver_FLASH0.ProgramData(magic_trailer_offset, magic_trailer_value, sizeof(magic_trailer_value));
  if (ret != ARM_DRIVER_OK){
    APP_DBG_MSG("Error with ProgramData: %i for magic offset: %x\r\n\n", ret, magic_trailer_offset);
    return -1;
  }
  
  return 0;
}
