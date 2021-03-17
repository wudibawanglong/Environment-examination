#include "flash.h"

void flash_erase_record(void)
{

	/* 如果不擦除，写入会失败，读取的数据都为0 */
//	printf("FLASH_EraseSector start\r\n");
	

	if (FLASH_EraseSector(FLASH_Sector_4, VoltageRange_3) != FLASH_COMPLETE)
	{ 
			//printf("Erase error\r\n");
			return;
	}

	//printf("FLASH_EraseSector ends\r\n");
}

uint32_t flash_write_record(char *pbuf,uint32_t record_count)
{
	uint32_t addr_start=ADDR_FLASH_SECTOR_4+record_count*64;
	uint32_t addr_end  =addr_start+64;

	uint32_t i=0;
	
	while (addr_start < addr_end)
	{
		//每次写入数据是4个字节
		if (FLASH_ProgramWord(addr_start, *(uint32_t *)&pbuf[i]) == FLASH_COMPLETE)
		{
			//地址每次偏移4个字节
			addr_start +=4;
			
			i+=4;
		}

		else
		{ 
			//printf("flash write record fail,now goto erase sector!\r\n");
			
			//重新擦除扇区
			flash_erase_record();

			return 1;
		}
	}
	return 0;
}

void flash_read_record(char *pbuf,uint32_t record_count)
{
	uint32_t addr_start=ADDR_FLASH_SECTOR_4+record_count*64;
	uint32_t addr_end  =addr_start+64;

	uint32_t i=0;
	
	while (addr_start < addr_end)
	{
		*(uint32_t *)&pbuf[i] = *(__IO uint32_t*)addr_start;

		addr_start+=4;
		
		i = i + 4;
	}

}


