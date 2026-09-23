#include "wdg.h"
#include "stdio.h"

/*
IWDG    
    IWDG的时钟为CK_IWDG=40kHz 
    计数器时钟CK_CNT=CK_IWDG/IWDG_PR
    超时时间Tout=1/CK_CNT*rlv， rlv是重装载值寄存器的值
*/
void Init_IWDG(uint8_t prv,uint16_t rlv){
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);   //使能IWDG_PR IWDG_RLR寄存器的写操作
    IWDG_SetPrescaler(prv);         //设置 IWDG 预分频值
    IWDG_SetReload(rlv);            //设置 IWDG 重装载值
    IWDG_ReloadCounter();           //把重装载值放到计数器中
    IWDG_Enable();                  //使能IWDG
}

void IWDG_Feed(void){
    IWDG_ReloadCounter();           //喂狗 把重装载值放到计数器中，防止计数器为0重启
}


/*
WWDG:
    如果启动了看门狗并且使能中断，当递减计数器等于0x40时会产生一个早期唤醒中断(EWI)
    这个中断被称为死前中断或者遗嘱中断，在中断函数中，应该处理最重要的事，而且必须要快
    因为计数器再递减一次就会产生系统复位信号

    WWDG的时钟来源PCLK1 - APB1
    tr:递减计时器的值  取值范围0x7F-0x40
    wr:窗口值，取值范围0x7F-0x40
    prv: 预分频值，取值范围
        WWDG_Prescaler_1 =（PCLK/4096）/1 
        WWDG_Prescaler_2 =（PCLK/4096）/2 
        WWDG_Prescaler_4 =（PCLK/4096）/4 
        WWDG_Prescaler_8 =（PCLK/4096）/8 
    eg:
        当tr = 0X7F ：计数器从0x7F开始递减
        当wr = 0X5F ：窗口的上限 ，窗口的下限是0x40
        此时如果再大于0x5F喂狗，或者当前计数小于0x40，都会产生复位信号
        只有再0X5F-0X40窗口时间内喂狗，才不会产生复位信号
        所以WWDG更适合在精确的时间周期来进行使用 
*/
void Init_WWDG(uint8_t tr,uint8_t wr,uint32_t prv){
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG,ENABLE); //使能看门狗时钟
    WWDG_SetPrescaler(prv);     //设置预分频值
    WWDG_SetWindowValue(wr);    //设置窗口上限
    WWDG_Enable(WWDG_CNT&tr);   //使能WWDG并装入计数器值 - 从什么值开始递减
    /*NVIC*/
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel=WWDG_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelCmd=ENABLE;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority=3;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority=0;
    NVIC_Init(&NVIC_InitStruct);
    
    WWDG_ClearFlag();   //清除标志位
    WWDG_EnableIT();    //使能中断
}

void WWDG_IRQHandler(void){         //当WWDG计数器减到0X40第一时间会产生此中断
    WWDG_SetCounter(WWDG_CNT);      //设置 WWDG 计数器值
    WWDG_ClearFlag();   
}

void WWDG_Feed(void){
    WWDG_SetCounter(WWDG_CNT);      //设置 WWDG 计数器值
}


