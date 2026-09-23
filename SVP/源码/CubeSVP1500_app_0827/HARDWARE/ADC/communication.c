#include "communication.h"
#include "spi.h"

#define HARD_SPI
/***************************************************************************//**
 * @brief Initializes the SPI communication peripheral.
 *
 * @param lsbFirst - Transfer format (0 or 1).
 *                   Example: 0x0 - MSB first.
 *                            0x1 - LSB first.
 * @param clockFreq - SPI clock frequency (Hz).
 *                    Example: 1000 - SPI clock frequency is 1 kHz.
 * @param clockPol - SPI clock polarity (0 or 1).
 *                   Example: 0x0 - Idle state for clock is a low level; active
 *                                  state is a high level;
 *	                      0x1 - Idle state for clock is a high level; active
 *                                  state is a low level.
 * @param clockEdg - SPI clock edge (0 or 1).
 *                   Example: 0x0 - Serial output data changes on transition
 *                                  from idle clock state to active clock state;
 *                            0x1 - Serial output data changes on transition
 *                                  from active clock state to idle clock state.
 *
 * @return status - Result of the initialization procedure.
 *                  Example: 1 - if initialization was successful;
 *                           0 - if initialization was unsuccessful.
*******************************************************************************/
unsigned char AD7124_SPI_Init(unsigned char lsbFirst,
                       unsigned long clockFreq,
                       unsigned char clockPol,
                       unsigned char clockEdg)
{
//    #ifdef HARD_SPI
	SPI1_Init();
	//SPI1_SetSpeed(SPI_BaudRatePrescaler_32);
//	#else
//	ADS7124_Init();
//	#endif
	
	return 0;
}

/***************************************************************************//**
 * @brief Reads data from SPI.
 *
 * @param slaveDeviceId - The ID of the selected slave device.
 * @param data - Data represents the write buffer as an input parameter and the
 *               read buffer as an output parameter.
 * @param bytesNumber - Number of bytes to read.
 *
 * @return Number of read bytes.
*******************************************************************************/
unsigned char AD7124_SPI_Read(unsigned char slaveDeviceId,
                       unsigned char* data,
                       unsigned char bytesNumber)
{
	u8 i=0;
	AD7124_CS = 0;
	SPI_ReadWriteByte(&SPI2_Config,data[0]);
	
	for(i=1; i<bytesNumber; i++)
	{
		data[i] = SPI_ReadWriteByte(&SPI2_Config,data[0]);
	}
	AD7124_CS = 1;
	
	return 0;
}

/***************************************************************************//**
 * @brief Writes data to SPI.
 *
 * @param slaveDeviceId - The ID of the selected slave device.
 * @param data - Data represents the write buffer.
 * @param bytesNumber - Number of bytes to write.
 *
 * @return Number of written bytes.
*******************************************************************************/
#include "usart.h"

unsigned char AD7124_SPI_Write(unsigned char slaveDeviceId,
                        unsigned char* data,
                        unsigned char bytesNumber)
{
	u8 i=0;
	AD7124_CS = 0;
	for(i=0; i<bytesNumber; i++)
	{
		SPI_ReadWriteByte(&SPI2_Config,data[i]);
	}
	AD7124_CS = 1;
	
	return 0;
}



