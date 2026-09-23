#include "./USMART/usmart.h"
#include "./USMART/usmart_str.h"

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"

struct _m_usmart_nametab usmart_nametab[] =
{
#if USMART_USE_WRFUNS == 1
    (void *)read_addr, "uint32_t read_addr(uint32_t addr)",
    (void *)write_addr, "void write_addr(uint32_t addr, uint32_t val)",
#endif
    (void *)delay_ms, "void delay_ms(uint16_t nms)",
    (void *)delay_us, "void delay_us(uint32_t nus)",
};

unsigned short usmart_nametab_size = sizeof(usmart_nametab) / sizeof(struct _m_usmart_nametab);

struct _m_usmart_dev usmart_dev =
{
    usmart_nametab,
    usmart_init,
    usmart_cmd_rec,
    usmart_exe,
    usmart_scan,
    sizeof(usmart_nametab) / sizeof(struct _m_usmart_nametab),
    0,
    0,
    1,
    0,
    {0},
    {0},
    0,
    0,
};
