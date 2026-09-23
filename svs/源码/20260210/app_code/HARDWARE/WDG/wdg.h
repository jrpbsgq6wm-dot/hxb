//IWDG
#ifndef __WDG_H__
#define __WDG_H__

#include "stm32f10x.h"

//IWDG
void Init_IWDG(uint8_t prv,uint16_t rlv);
void IWDG_Feed(void);

//WWDG

#define WWDG_CNT    0X7F
void Init_WWDG(uint8_t tr,uint8_t wr,uint32_t prv);
void WWDG_Feed(void);



#endif



