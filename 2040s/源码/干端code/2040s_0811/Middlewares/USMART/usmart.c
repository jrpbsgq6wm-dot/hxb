/**
 ****************************************************************************************************
 * @file        usmart.c
 * @author            ()
 * @version     V3.5
 * @date        2020-12-20
 * @brief       USMART                   
 *
 *              USMART                                                      ,          ,                                    
 *                                      ,         .      ,                                          (            (10/16      ,            )
 *                                                              ),                        10               ,                               .
 *              V2.1                  hex   dec            .                                                   .                           
 *                       ,      :
 *                    "hex 100"                                    HEX 0X64.
 *                    "dec 0X64"                                   DEC 100.
 *   @note
 *              USMART                  @MDK 3.80A@2.0         
 *              FLASH:4K~K      (      USMART_USE_HELP   USMART_USE_WRFUNS      )
 *              SRAM:72      (                  )
 *              SRAM            :   SRAM=PARM_LEN+72-4        PARM_LEN                  4.
 *                                         100         .
 * @license     Copyright (c) 2020-2032,                                        
 ****************************************************************************************************
 * @attention
 * 
 *             :www.yuanzige.com
 *             :www.openedv.com
 *             :www.alientek.com
 *             :openedv.taobao.com
 *
 *              
 * 
 * V3.4                                 USMART               :readme.txt
 * 
 * V3.4 20200324
 * 1,       usmart_port.c   usmart_port.h,            USMART         ,            
 * 2,                            : uint8_t, uint16_t, uint32_t
 * 3,       usmart_reset_runtime   usmart_timx_reset_time
 * 4,       usmart_get_runtime   usmart_timx_get_time
 * 5,       usmart_scan                  ,         usmart_get_input_string               
 * 6,       printf         USMART_PRINTF         
 * 7,                               ,                     ,            
 *
 * V3.5 20201220
 * 1                              AC6         
 *
 ****************************************************************************************************
 */

#include "./USMART/usmart.h"
#include "./USMART/usmart_str.h"
#include "./USMART/usmart_port.h"


/*              */
char *sys_cmd_tab[] =
{
    "?",
    "help",
    "list",
    "id",
    "hex",
    "dec",
    "runtime",
};

/**
 * @brief                         
 * @param       str :                
 * @retval      0,            ;      ,            ;
 */
uint8_t usmart_sys_cmd_exe(char *str)
{
    uint8_t i;
    char sfname[MAX_FNAME_LEN];                  /*                       */
    uint8_t pnum;
    uint8_t rval;
    uint32_t res;
    res = usmart_get_cmdname(str, sfname, &i, MAX_FNAME_LEN);   /*                             */

    if (res)return USMART_FUNCERR;                  /*                 */

    str += i;

    for (i = 0; i < sizeof(sys_cmd_tab) / 4; i++)   /*                       */
    {
        if (usmart_strcmp(sfname, sys_cmd_tab[i]) == 0)break;
    }

    switch (i)
    {
        case 0:
        case 1: /*              */
            USMART_PRINTF("\r\n");
#if USMART_USE_HELP
            USMART_PRINTF("------------------------USMART V3.5------------------------ \r\n");
            USMART_PRINTF("    USMART                                                      ,       \r\n");
            USMART_PRINTF("   ,                                                            ,         .      ,      \r\n");
            USMART_PRINTF("                                    (            (10/16      ,            )            \r\n"),
            USMART_PRINTF("                                    ),                        10               ,         \r\n"),
            USMART_PRINTF("                     .                                    ,                        .\r\n");
            USMART_PRINTF("            :www.openedv.com\r\n");
            USMART_PRINTF("USMART   7               (            ):\r\n");
            USMART_PRINTF("?:                        \r\n");
            USMART_PRINTF("help:                     \r\n");
            USMART_PRINTF("list:                        \r\n\n");
            USMART_PRINTF("id:                    ID      \r\n\n");
            USMART_PRINTF("hex:          16            ,            +                           \r\n\n");
            USMART_PRINTF("dec:          10            ,            +                           \r\n\n");
            USMART_PRINTF("runtime:1,                        ;0,                        ;\r\n\n");
            USMART_PRINTF("                                                                        .\r\n");
            USMART_PRINTF("--------------------------------------------------- \r\n");
#else
            USMART_PRINTF("            \r\n");
#endif
            break;

        case 2: /*              */
            USMART_PRINTF("\r\n");
            USMART_PRINTF("-------------------------            --------------------------- \r\n");

            for (i = 0; i < usmart_dev.fnum; i++)USMART_PRINTF("%s\r\n", usmart_dev.funs[i].name);

            USMART_PRINTF("\r\n");
            break;

        case 3: /*       ID */
            USMART_PRINTF("\r\n");
            USMART_PRINTF("-------------------------       ID --------------------------- \r\n");

            for (i = 0; i < usmart_dev.fnum; i++)
            {
                usmart_get_fname((char *)usmart_dev.funs[i].name, sfname, &pnum, &rval); /*                       */
                USMART_PRINTF("%s id is:\r\n0X%08X\r\n", sfname, (unsigned int)usmart_dev.funs[i].func);  /*       ID */
            }

            USMART_PRINTF("\r\n");
            break;

        case 4: /* hex       */
            USMART_PRINTF("\r\n");
            usmart_get_aparm(str, sfname, &i);

            if (i == 0) /*              */
            {
                i = usmart_str2num(sfname, &res);       /*                 */

                if (i == 0) /*                    */
                {
                    USMART_PRINTF("HEX:0X%X\r\n", res); /*       16       */
                }
                else if (i != 4)return USMART_PARMERR;  /*             . */
                else        /*                          */
                {
                    USMART_PRINTF("16                  !\r\n");
                    usmart_dev.sptype = SP_TYPE_HEX;
                }

            }
            else return USMART_PARMERR; /*             . */

            USMART_PRINTF("\r\n");
            break;

        case 5: /* dec       */
            USMART_PRINTF("\r\n");
            usmart_get_aparm(str, sfname, &i);

            if (i == 0)     /*              */
            {
                i = usmart_str2num(sfname, &res);       /*                 */

                if (i == 0) /*                    */
                {
                    USMART_PRINTF("DEC:%lu\r\n", (unsigned long)res);  /*       10       */
                }
                else if (i != 4)
                {
                    return USMART_PARMERR;  /*             . */
                }
                else        /*                          */
                {
                    USMART_PRINTF("10                  !\r\n");
                    usmart_dev.sptype = SP_TYPE_DEC;
                }

            }
            else 
            {
                return USMART_PARMERR;  /*             . */
            }
                
            USMART_PRINTF("\r\n");
            break;

        case 6: /* runtime      ,                                     */
            USMART_PRINTF("\r\n");
            usmart_get_aparm(str, sfname, &i);

            if (i == 0) /*              */
            {
                i = usmart_str2num(sfname, &res);   /*                 */

                if (i == 0) /*                                */
                {
                    if (USMART_ENTIMX_SCAN == 0)
                    {
                        USMART_PRINTF("\r\nError! \r\nTo EN RunTime function,Please set USMART_ENTIMX_SCAN = 1 first!\r\n");/*        */
                    }
                    else
                    {
                        usmart_dev.runtimeflag = res;

                        if (usmart_dev.runtimeflag)
                        {
                            USMART_PRINTF("Run Time Calculation ON\r\n");
                        }
                        else 
                        {
                            USMART_PRINTF("Run Time Calculation OFF\r\n");
                        }
                    }
                }
                else 
                {
                    return USMART_PARMERR;  /*             ,                   */
                }
            }
            else 
            {
                return USMART_PARMERR;      /*             . */
            }
            
            USMART_PRINTF("\r\n");
            break;

        default:/*              */
            return USMART_FUNCERR;
    }

    return 0;
}

/**
 * @brief                USMART
 * @param       tclk:                         (      :Mhz)
 * @retval         
 */
void usmart_init(uint16_t tclk)
{
#if USMART_ENTIMX_SCAN == 1
    usmart_timx_init(1000, tclk * 100 - 1);
#endif
    usmart_dev.sptype = 1;  /*                          */
}

/**
 * @brief          str                  ,id,               
 * @param       str:                .
 * @retval      0,            ;      ,            .
 */
uint8_t usmart_cmd_rec(char *str)
{
    uint8_t sta, i, rval;   /*        */
    uint8_t rpnum, spnum;
    char rfname[MAX_FNAME_LEN];  /*             ,                                  */
    char sfname[MAX_FNAME_LEN];  /*                       */
    sta = usmart_get_fname(str, rfname, &rpnum, &rval); /*                                                     */

    if (sta)return sta; /*        */

    for (i = 0; i < usmart_dev.fnum; i++)
    {
        sta = usmart_get_fname((char *)usmart_dev.funs[i].name, sfname, &spnum, &rval); /*                                      */

        if (sta)return sta; /*                    */

        if (usmart_strcmp(sfname, rfname) == 0) /*        */
        {
            if (spnum > rpnum)return USMART_PARMERR;/*             (                                 ) */

            usmart_dev.id = i;  /*             ID. */
            break;  /*       . */
        }
    }

    if (i == usmart_dev.fnum)return USMART_NOFUNCFIND;  /*                          */

    sta = usmart_get_fparam(str, &i);   /*                          */

    if (sta)return sta;     /*              */

    usmart_dev.pnum = i;    /*                    */
    return USMART_OK;
}

/**
 * @brief       USMART            
 *   @note
 *                                                                       .
 *                          10                  ,                                       .                  .      5                                             .
 *                                                     .   :"         (      1         2...      N)=         ".               .
 *                                                           ,                                                .
 *
 * @param          
 * @retval         
 */
void usmart_exe(void)
{
    uint8_t id, i;
    uint32_t res;
    uint32_t temp[MAX_PARM];        /*             ,                         */
    char sfname[MAX_FNAME_LEN];  /*                       */
    uint8_t pnum, rval;
    id = usmart_dev.id;

    if (id >= usmart_dev.fnum)return;   /*          . */

    usmart_get_fname((char *)usmart_dev.funs[id].name, sfname, &pnum, &rval);    /*                      ,                */
    USMART_PRINTF("\r\n%s(", sfname);   /*                                */

    for (i = 0; i < pnum; i++)      /*              */
    {
        if (usmart_dev.parmtype & (1 << i)) /*                    */
        {
            USMART_PRINTF("%c", '"');
            USMART_PRINTF("%s", usmart_dev.parm + usmart_get_parmpos(i));
            USMART_PRINTF("%c", '"');
            temp[i] = (uint32_t) & (usmart_dev.parm[usmart_get_parmpos(i)]);
        }
        else    /*                 */
        {
            temp[i] = *(uint32_t *)(usmart_dev.parm + usmart_get_parmpos(i));

            if (usmart_dev.sptype == SP_TYPE_DEC)
            {
                USMART_PRINTF("%ld", (long)temp[i]);  /* 10                   */
            }
            else 
            {
                USMART_PRINTF("0X%X", temp[i]); /* 16                   */
            }
        }

        if (i != pnum - 1)USMART_PRINTF(",");
    }

    USMART_PRINTF(")");
#if USMART_ENTIMX_SCAN==1
    usmart_timx_reset_time();   /*                ,             */
#endif

    switch (usmart_dev.pnum)
    {
        case 0: /*          (void      ) */
            res = (*(uint32_t(*)())usmart_dev.funs[id].func)();
            break;

        case 1: /*    1          */
            res = (*(uint32_t(*)())usmart_dev.funs[id].func)(temp[0]);
            break;

        case 2: /*    2          */
            res = (*(uint32_t(*)())usmart_dev.funs[id].func)(temp[0], temp[1]);
            break;

        case 3: /*    3          */
            res = (*(uint32_t(*)())usmart_dev.funs[id].func)(temp[0], temp[1], temp[2]);
            break;

        case 4: /*    4          */
            res = (*(uint32_t(*)())usmart_dev.funs[id].func)(temp[0], temp[1], temp[2], temp[3]);
            break;

        case 5: /*    5          */
            res = (*(uint32_t(*)())usmart_dev.funs[id].func)(temp[0], temp[1], temp[2], temp[3], temp[4]);
            break;

        case 6: /*    6          */
            res = (*(uint32_t(*)())usmart_dev.funs[id].func)(temp[0], temp[1], temp[2], temp[3], temp[4], \
                    temp[5]);
            break;

        case 7: /*    7          */
            res = (*(uint32_t(*)())usmart_dev.funs[id].func)(temp[0], temp[1], temp[2], temp[3], temp[4], \
                    temp[5], temp[6]);
            break;

        case 8: /*    8          */
            res = (*(uint32_t(*)())usmart_dev.funs[id].func)(temp[0], temp[1], temp[2], temp[3], temp[4], \
                    temp[5], temp[6], temp[7]);
            break;

        case 9: /*    9          */
            res = (*(uint32_t(*)())usmart_dev.funs[id].func)(temp[0], temp[1], temp[2], temp[3], temp[4], \
                    temp[5], temp[6], temp[7], temp[8]);
            break;

        case 10:/*    10          */
            res = (*(uint32_t(*)())usmart_dev.funs[id].func)(temp[0], temp[1], temp[2], temp[3], temp[4], \
                    temp[5], temp[6], temp[7], temp[8], temp[9]);
            break;
    }

#if USMART_ENTIMX_SCAN==1
    usmart_timx_get_time(); /*                          */
#endif

    if (rval == 1)  /*                . */
    {
        if (usmart_dev.sptype == SP_TYPE_DEC)USMART_PRINTF("=%lu;\r\n", (unsigned long)res);   /*                   (10                  ) */
        else USMART_PRINTF("=0X%X;\r\n", res);  /*                   (16                  ) */
    }
    else USMART_PRINTF(";\r\n");    /*                   ,                   */

    if (usmart_dev.runtimeflag)     /*                                */
    {
        USMART_PRINTF("Function Run Time:%d.%1dms\r\n", usmart_dev.runtime / 10, usmart_dev.runtime % 10);  /*                          */
    }
}

/**
 * @brief       USMART            
 *   @note
 *                                   ,      USMART               .                                                
 *                                                              .
 *                                                  ,                        .
 *                             ,   USART_RX_STA   USART_RX_BUF[]                        
 *
 * @param          
 * @retval         
 */
void usmart_scan(void)
{
    uint8_t sta, len;
    char *pbuf = 0;

    pbuf = usmart_get_input_string();   /*                       */
    if (pbuf == 0) return ; /*             ,              */
     
    sta = usmart_dev.cmd_rec(pbuf);     /*                          */

    if (sta == 0)
    {
        usmart_dev.exe();  /*              */
    }
    else
    {
        len = usmart_sys_cmd_exe(pbuf);

        if (len != USMART_FUNCERR)sta = len;

        if (sta)
        {
            switch (sta)
            {
                case USMART_FUNCERR:
                    USMART_PRINTF("            !\r\n");
                    break;

                case USMART_PARMERR:
                    USMART_PRINTF("            !\r\n");
                    break;

                case USMART_PARMOVER:
                    USMART_PRINTF("            !\r\n");
                    break;

                case USMART_NOFUNCFIND:
                    USMART_PRINTF("                        !\r\n");
                    break;
            }
        }
    } 
 
}

#if USMART_USE_WRFUNS == 1  /*                             */

/**
 * @brief                                
 * @param          
 * @retval         
 */ 
uint32_t read_addr(uint32_t addr)
{
    return *(uint32_t *)addr;
}

/**
 * @brief                                        
 * @param          
 * @retval         
 */ 
void write_addr(uint32_t addr, uint32_t val)
{
    *(uint32_t *)addr = val;
}

#endif





















