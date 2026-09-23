/**
 ****************************************************************************************************
 * @file        usmart_str.c
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

/**
 * @brief                      str1   str2
 * @param       str1:          1         (      )
 * @param       str2:          2         (      )
 * @retval      0         ; 1            ;
 */
uint8_t usmart_strcmp(char *str1, char *str2)
{
    while (1)
    {
        if (*str1 != *str2)return 1; /*           */

        if (*str1 == '\0')break;    /*                . */

        str1++;
        str2++;
    }

    return 0;/*                       */
}

/**
 * @brief          src         copy   dst
 * @param       src:          
 * @param       dst:             
 * @retval      0         ; 1            ;
 */
void usmart_strcopy(char *src, char *dst)
{
    while (1)
    {
        *dst = *src;            /*        */

        if (*src == '\0')break; /*                . */

        src++;
        dst++;
    }
}

/**
 * @brief                               (      )
 * @param       str:                
 * @retval                        
 */
uint8_t usmart_strlen(char *str)
{
    uint8_t len = 0;

    while (1)
    {
        if (*str == '\0')break; /*                . */

        len++;
        str++;
    }

    return len;
}

/**
 * @brief                   , m^n
 * @param       m:       
 * @param       n:       
 * @retval      m   n      
 */
uint32_t usmart_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;

    while (n--)result *= m;

    return result;
}

/**
 * @brief                               
 *   @note
 *                    16            ,      16                              ,               0X         .
 *                          
 * @param       str:                
 * @param       res:                               .
 * @retval                  :
 *   @arg       0,                   
 *   @arg       1,                   
 *   @arg       2, 16               0
 *   @arg       3,                   
 *   @arg       4,                   0
 */
uint8_t usmart_str2num(char *str, uint32_t *res)
{
    uint32_t t;
    int tnum;
    uint8_t bnum = 0;   /*                 */
    char *p;
    uint8_t hexdec = 10;/*                          */
    uint8_t flag = 0;   /* 0,                  ;1,            ;2,            . */
    p = str;
    *res = 0;   /*       . */

    while (1)
    {
        /*                       */
        if ((*p <= '9' && *p >= '0') || ((*str == '-' || *str == '+') && bnum == 0) || (*p <= 'F' && *p >= 'A') || (*p == 'X' && bnum == 1))
        {
            if (*p >= 'A')hexdec = 16;  /*                         ,   16            . */

            if (*str == '-')
            {
                flag = 2;   /*                 */
                str += 1;
            }
            else if (*str == '+')
            {
                flag = 1;   /*                 */
                str += 1;
            }
            else
            {
                bnum++; /*             . */
            }
        }
        else if (*p == '\0')
        {
            break;      /*                ,       */
        }
        else
        {
            return 1;   /*                         16            . */
        }

        p++;
    }

    p = str;            /*                                        . */

    if (hexdec == 16)   /* 16             */
    {
        if (bnum < 3)return 2;  /*             3               .      0X         2   ,      0X                  ,                  . */

        if (*p == '0' && (*(p + 1) == 'X'))   /*          '0X'      . */
        {
            p += 2;     /*                            . */
            bnum -= 2;  /*                 */
        }
        else
        {
            return 3;   /*                          */
        }
    }
    else if (bnum == 0)
    {
        return 4;       /*          0               . */
    }

    while (1)
    {
        if (bnum)bnum--;

        if (*p <= '9' && *p >= '0')t = *p - '0';    /*                    */
        else t = *p - 'A' + 10; /*       A~F             */

        *res += t * usmart_pow(hexdec, bnum);
        p++;

        if (*p == '\0')break;   /*                   . */
    }

    if (flag == 2)      /*          ? */
    {
        tnum = -*res;
        *res = tnum;
    }

    return 0;   /*              */
}

/**
 * @brief                      
 * @param       str     :             
 * @param       cmdname :          
 * @param       nlen    :                
 * @param       maxlen  :             (         ,                        )
 * @retval      0,      ;      ,      .
 */
uint8_t usmart_get_cmdname(char *str, char *cmdname, uint8_t *nlen, uint8_t maxlen)
{
    *nlen = 0;

    while (*str != ' ' && *str != '\0')   /*                                               */
    {
        *cmdname = *str;
        str++;
        cmdname++;
        (*nlen)++;  /*                    */

        if (*nlen >= maxlen)return 1;   /*                 */
    }

    *cmdname = '\0';/*                 */
    return 0;       /*              */
}

/**
 * @brief                                                                                                                                        
 * @param       str :                
 * @retval                     
 */
uint8_t usmart_search_nextc(char *str)
{
    str++;

    while (*str == ' ' && str != 0)str++;

    return *str;
}

/**
 * @brief          str                  
 * @param       str   :                   
 * @param       fname :                               
 * @param       pnum  :                      
 * @param       rval  :                            (0,         ;1,      )
 * @retval      0,      ;      ,            .
 */
uint8_t usmart_get_fname(char *str, char *fname, uint8_t *pnum, uint8_t *rval)
{
    uint8_t res;
    uint8_t fover = 0;  /*              */
    char *strtemp;
    uint8_t offset = 0;
    uint8_t parmnum = 0;
    uint8_t temp = 1;
    char fpname[6];  /* void+X+'/0' */
    uint8_t fplcnt = 0; /*                                   */
    uint8_t pcnt = 0;   /*                 */
    uint8_t nchar;
    /*                                */
    strtemp = str;

    while (*strtemp != '\0')    /*              */
    {
        if (*strtemp != ' ' && (pcnt & 0X7F) < 5)   /*             5          */
        {
            if (pcnt == 0)pcnt |= 0X80; /*                ,                                  */

            if (((pcnt & 0x7f) == 4) && (*strtemp != '*'))break;    /*                   ,         * */

            fpname[pcnt & 0x7f] = *strtemp; /*                                */
            pcnt++;
        }
        else if (pcnt == 0X85)
        {
            break;
        }

        strtemp++;
    }

    if (pcnt)   /*              */
    {
        fpname[pcnt & 0x7f] = '\0'; /*                 */

        if (usmart_strcmp(fpname, "void") == 0)
        {
            *rval = 0;  /*                    */
        }
        else
        {
            *rval = 1;  /*                 */
        }

        pcnt = 0;
    }

    res = 0;
    strtemp = str;

    while (*strtemp != '(' && *strtemp != '\0')   /*                                               */
    {
        strtemp++;
        res++;

        if (*strtemp == ' ' || *strtemp == '*')
        {
            nchar = usmart_search_nextc(strtemp);   /*                       */

            if (nchar != '(' && nchar != '*')offset = res;  /*                *    */
        }
    }

    strtemp = str;

    if (offset)strtemp += offset + 1;   /*                                */

    res = 0;
    nchar = 0;  /*                                     ,0                  ;1               ; */

    while (1)
    {
        if (*strtemp == 0)
        {
            res = USMART_FUNCERR;   /*              */
            break;
        }
        else if (*strtemp == '(' && nchar == 0)
        {
            fover++;    /*                          */
        }
        else if (*strtemp == ')' && nchar == 0)
        {
            if (fover)
            {
                fover--;
            }
            else
            {
                res = USMART_FUNCERR;  /*             ,         '(' */
            }

            if (fover == 0)break;       /*             ,       */
        }
        else if (*strtemp == '"')
        {
            nchar = !nchar;
        }

        if (fover == 0)   /*                          */
        {
            if (*strtemp != ' ')    /*                          */
            {
                *fname = *strtemp;  /*                 */
                fname++;
            }
        }
        else     /*                               . */
        {
            if (*strtemp == ',')
            {
                temp = 1;           /*                          */
                pcnt++;
            }
            else if (*strtemp != ' ' && *strtemp != '(')
            {
                if (pcnt == 0 && fplcnt < 5)    /*                         ,                  void               ,               . */
                {
                    fpname[fplcnt] = *strtemp;  /*                   . */
                    fplcnt++;
                }

                temp++;     /*                   (         ) */
            }

            if (fover == 1 && temp == 2)
            {
                temp++;     /*                    */
                parmnum++;  /*                    */
            }
        }

        strtemp++;
    }

    if (parmnum == 1)       /*       1         . */
    {
        fpname[fplcnt] = '\0';  /*                 */

        if (usmart_strcmp(fpname, "void") == 0)parmnum = 0; /*          void,                  . */
    }

    *pnum = parmnum;/*                    */
    *fname = '\0';  /*                 */
    return res;     /*                    */
}

/**
 * @brief          str                              
 * @param       str   :                   
 * @param       fparm :                      
 * @param       ptype :             
 *   @arg       0            
 *   @arg       1               
 *   @arg       0XFF               
 * @retval
 *   @arg       0,                     
 *   @arg             ,                           .
 */
uint8_t usmart_get_aparm(char *str, char *fparm, uint8_t *ptype)
{
    uint8_t i = 0;
    uint8_t enout = 0;
    uint8_t type = 0;   /*                 */
    uint8_t string = 0; /*       str                */

    while (1)
    {
        if (*str == ',' && string == 0)enout = 1;   /*                   ,                                              */

        if ((*str == ')' || *str == '\0') && string == 0)break; /*                       */

        if (type == 0)   /*                    */
        {
            /*                 */
            if ((*str >= '0' && *str <= '9') || *str == '-' || *str == '+' || (*str >= 'a' && *str <= 'f') || (*str >= 'A' && *str <= 'F') || *str == 'X' || *str == 'x')
            {
                if (enout)break;    /*                         ,            . */

                if (*str >= 'a')
                {
                    *fparm = *str - 0X20;   /*                       */
                }
                else
                {
                    *fparm = *str; /*                                */
                }

                fparm++;
            }
            else if (*str == '"')     /*                                */
            {
                if (enout)break;    /*       ,            ",               . */

                type = 1;
                string = 1;         /*       STRING              */
            }
            else if (*str != ' ' && *str != ',')     /*                   ,             */
            {
                type = 0XFF;
                break;
            }
        }
        else     /* string    */
        {
            if (*str == '"')string = 0;

            if (enout)break;    /*                         ,            . */

            if (string)         /*                    */
            {
                if (*str == '\\')   /*                (                  ) */
                {
                    str++;      /*                                  ,                  ,      COPY */
                    i++;
                }

                *fparm = *str;  /*                                */
                fparm++;
            }
        }

        i++;    /*                 */
        str++;
    }

    *fparm = '\0';  /*                 */
    *ptype = type;  /*                    */
    return i;       /*                    */
}

/**
 * @brief                                        
 * @param       num   :    num         ,      0~9.
 * @retval                              
 */
uint8_t usmart_get_parmpos(uint8_t num)
{
    uint8_t temp = 0;
    uint8_t i;

    for (i = 0; i < num; i++)
    {
        temp += usmart_dev.plentbl[i];
    }

    return temp;
}

/**
 * @brief          str                     
 * @param       str  :             
 * @param       parn :                .0                void      
 * @retval      0,      ;      ,            .
 */
uint8_t usmart_get_fparam(char *str, uint8_t *parn)
{
    uint8_t i, type;
    uint32_t res;
    uint8_t n = 0;
    uint8_t len;
    char tstr[PARM_LEN + 1]; /*                      ,                  PARM_LEN                      */

    for (i = 0; i < MAX_PARM; i++)
    {
        usmart_dev.plentbl[i] = 0;  /*                       */
    }

    while (*str != '(')   /*                                */
    {
        str++;

        if (*str == '\0')return USMART_FUNCERR; /*                    */
    }

    str++;  /*          "("                         */

    while (1)
    {
        i = usmart_get_aparm(str, tstr, &type); /*                       */
        str += i;   /*        */

        switch (type)
        {
            case 0: /*        */
                if (tstr[0] != '\0')    /*                          */
                {
                    i = usmart_str2num(tstr, &res); /*                 */

                    if (i)return USMART_PARMERR;    /*             . */

                    *(uint32_t *)(usmart_dev.parm + usmart_get_parmpos(n)) = res;   /*                            . */
                    usmart_dev.parmtype &= ~(1 << n);   /*              */
                    usmart_dev.plentbl[n] = 4;  /*                      4 */
                    n++;    /*              */

                    if (n > MAX_PARM)return USMART_PARMOVER;    /*              */
                }

                break;

            case 1:/*           */
                len = usmart_strlen(tstr) + 1;  /*                   '\0' */
                usmart_strcopy(tstr, (char *)&usmart_dev.parm[usmart_get_parmpos(n)]);  /*       tstr         usmart_dev.parm[n] */
                usmart_dev.parmtype |= 1 << n;  /*                 */
                usmart_dev.plentbl[n] = len;    /*                      len */
                n++;

                if (n > MAX_PARM)return USMART_PARMOVER;    /*              */

                break;

            case 0XFF:/*        */
                return USMART_PARMERR;  /*              */
        }

        if (*str == ')' || *str == '\0')break;  /*                      . */
    }

    *parn = n;  /*                       */
    return USMART_OK;   /*                       */
}














