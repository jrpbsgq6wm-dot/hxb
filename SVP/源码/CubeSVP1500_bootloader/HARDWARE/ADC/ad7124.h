#ifndef _AD7124_H
#define _AD7124_H
/*
    Config
    ADC�Ĵ������ܣ�AD7124��8�ֶ������á�ÿ�����ð���һ���ĸ��Ĵ���
        ���üĴ���config
            ѡ��ADC������룺˫���ԣ�ADC���Բɼ�������ѹ- ����-VREF��/Gain��~VREF/Gain ���򵥼��ԣ�ADCֻ�ܲɼ�������ѹ��
            ������������£������ѹ������AVDD��AVSS��Χ��
            �����ѹ�ο�Դ
                1 �ڲ�2.5v�ο�
                2 ������REFIN1(+)��REFIN1(-)֮����ⲿ��׼��ѹԴ
                3 ������REFIN2(+)��REFIN2(-)֮����ⲿ��׼��ѹԴ��AVDD��AVSS��
                4 ����������PGA���棬�ṩ����ѡ��Ϊ1��2��4��8��16��32��64��128��
            ģ�����뻺�����ͻ�׼��ѹ���뻺����Ҳ�����øüĴ���ʹ�ܡ�
        �˲��Ĵ���filter
            24bit�Ĵ��� ��ѡΪ�����˲���������ADC��������������˲��������ͺ������������ͨ�����ô˼Ĵ����ĸ�λ��ѡ��
        ����Ĵ���Gain
            ����ADC����У׼ϵ����24λ�Ĵ���
        ƫ�üĴ���offset
            ʧ���Ĵ�������ADC��ʧ��У׼ϵ����ʧ���Ĵ������ϵ縴λֵΪ0x800000��ʧ���Ĵ���Ϊ24λ��/д�Ĵ���������û������ڲ���ϵͳ���ƽУ׼������д��ʧ���Ĵ������ϵ縴λֵ�����Զ����ǡ�
*/
#include <stdint.h>
#include "sys.h"  
#include "spi.h"
#include "usart.h"
#include "tdc_gp22.h"



#define AD7124_CS       PBout(12)

#define AD7124_4 0x00

#define Multi_Channel
#define Single_Channel
#define Vref 3300.0			 //�ڲ��ο���ѹ2.5V
#define AD_Tim_Interval 1000 //��λus

#define Samprate_2Hz		0x01 
#define Samprate_5Hz		0x02 
#define Samprate_10Hz		0x03 
#define Samprate_20Hz		0x04 
#define Samprate_50Hz		0x05 
#define Samprate_100Hz		0x06 
#define Samprate_200Hz		0x07 
#define Samprate_500Hz		0x08 
#define Samprate_1kHz		0x09

/******************************************************************************/
/******************* Register map and register definitions ********************/
/******************************************************************************/
 

/* AD7124 Register Map */
#define AD7124_COMM_REG      0x00
#define AD7124_STATUS_REG    0x00
#define AD7124_ADC_CTRL_REG  0x01
#define AD7124_DATA_REG      0x02
#define AD7124_IO_CTRL1_REG  0x03
#define AD7124_IO_CTRL2_REG  0x04
#define AD7124_ID_REG        0x05
#define AD7124_ERR_REG       0x06
#define AD7124_ERREN_REG     0x07
#define AD7124_CH0_MAP_REG   0x09
#define AD7124_CH1_MAP_REG   0x0A
#define AD7124_CH2_MAP_REG   0x0B
#define AD7124_CH3_MAP_REG   0x0C
#define AD7124_CH4_MAP_REG   0x0D
#define AD7124_CH5_MAP_REG   0x0E
#define AD7124_CH6_MAP_REG   0x0F
#define AD7124_CH7_MAP_REG   0x10
#define AD7124_CH8_MAP_REG   0x11
#define AD7124_CH9_MAP_REG   0x12
#define AD7124_CH10_MAP_REG  0x13
#define AD7124_CH11_MAP_REG  0x14
#define AD7124_CH12_MAP_REG  0x15
#define AD7124_CH13_MAP_REG  0x16
#define AD7124_CH14_MAP_REG  0x17
#define AD7124_CH15_MAP_REG  0x18
#define AD7124_CFG0_REG      0x19
#define AD7124_CFG1_REG      0x1A
#define AD7124_CFG2_REG      0x1B
#define AD7124_CFG3_REG      0x1C
#define AD7124_CFG4_REG      0x1D
#define AD7124_CFG5_REG      0x1E
#define AD7124_CFG6_REG      0x1F
#define AD7124_CFG7_REG      0x20
#define AD7124_FILT0_REG     0x21
#define AD7124_FILT1_REG     0x22
#define AD7124_FILT2_REG     0x23
#define AD7124_FILT3_REG     0x24
#define AD7124_FILT4_REG     0x25
#define AD7124_FILT5_REG     0x26
#define AD7124_FILT6_REG     0x27
#define AD7124_FILT7_REG     0x28
#define AD7124_OFFS0_REG     0x29
#define AD7124_OFFS1_REG     0x2A
#define AD7124_OFFS2_REG     0x2B
#define AD7124_OFFS3_REG     0x2C
#define AD7124_OFFS4_REG     0x2D
#define AD7124_OFFS5_REG     0x2E
#define AD7124_OFFS6_REG     0x2F
#define AD7124_OFFS7_REG     0x30
#define AD7124_GAIN0_REG     0x31
#define AD7124_GAIN1_REG     0x32
#define AD7124_GAIN2_REG     0x33
#define AD7124_GAIN3_REG     0x34
#define AD7124_GAIN4_REG     0x35
#define AD7124_GAIN5_REG     0x36
#define AD7124_GAIN6_REG     0x37
#define AD7124_GAIN7_REG     0x38

/* Communication Register bits  - ͨ�żĴ��� */
#define AD7124_COMM_REG_WEN    (0 << 7)
#define AD7124_COMM_REG_WR     (0 << 6)
#define AD7124_COMM_REG_RD     (1 << 6)
#define AD7124_COMM_REG_RA(x)  ((x) & 0x3F)

/* Status Register bits */
#define AD7124_STATUS_REG_RDY          (1 << 7)         //ADC����λ 0
#define AD7124_STATUS_REG_ERROR_FLAG   (1 << 6)         //ADC����λ��
#define AD7124_STATUS_REG_POR_FLAG     (1 << 4)         //�ϵ縴λ��־����λ��ʾ�����ϵ縴λ 1
#define AD7124_STATUS_REG_CH_ACTIVE(x) ((x) & 0xF)      //��Щλ��ʾADC���ڶ���һͨ��ִ��ת��������

/* ADC_Control Register bits - ���ƼĴ��� */
#define AD7124_ADC_CTRL_REG_DOUT_RDY_DEL   (1 << 12)    //����SCLK��Ч�ص�DOUT/RDY�ߵ�ƽʱ�� 0��10ns��-1(100ns)
#define AD7124_ADC_CTRL_REG_CONT_READ      (1 << 11)    //������ȡ���ݼĴ��� - 1
#define AD7124_ADC_CTRL_REG_DATA_STATUS    (1 << 10)    //ÿ�����ݼĴ���������֮��״̬�Ĵ������ݴ����ʹ��λ 
#define AD7124_ADC_CTRL_REG_CS_EN          (1 << 9)     //��λ�������ݶ�ȡ�����ڼ�DOUT��1��/ RDY��0�����ź�ʱ��DOUT���ű�ΪRDY���š�
#define AD7124_ADC_CTRL_REG_REF_EN         (1 << 8)     //�ڲ���׼��ѹʹ�ܣ���λ��1ʱ���ڲ���׼��ѹԴʹ�ܣ���ͨ��REFOUT���������
#define AD7124_ADC_CTRL_REG_POWER_MODE(x)  (((x) & 0x3) << 6)  
    /*
        ����ģʽѡ��
        00 �͹���   01 �й���  10 ȫ���ʹ��ʹ���   11ȫ����
    */
#define AD7124_ADC_CTRL_REG_MODE(x)        (((x) & 0xF) << 2)  
    /*
         //ADC�Ĺ���ģʽ
         0000 ����ת��ģʽ    -ADC����ִ��ת��-def
         0001 ����ת��ģʽ    -ADC�ϵ粢��ѡ��ͨ����ִ��ת��
         0010 ����ģʽ        -�ڲ���׼��ѹԴ��Ƭ���������Ͷ˹��ʿ��غ�ƫ�õ�ѹ�������ڴ���ģʽ�¿���ʹ�ܻ���á�Ƭ�ڼĴ����ڴ���ģʽ�±��������ݡ�
         0011 �ض�ģʽ        -���еĵ�·���ر� Ƭ�ڼĴ������������� ���мĴ������������±��
         0100 ����ģʽ        -�ڿ���ģʽ�� ADC�˲����͵����������ڸ�λ״̬
         0101 �ڲ����ƽ(ʧ��)У׼  -�ڲ���·�Զ����ӵ�����ˣ�'RDY��У׼����ʱ��Ϊ�ߵ�ƽ����У׼���ʱ���ص͵�ƽ��У׼������ɺ�ADC���ڿ���ģʽ����õ�ʧ��ϵ����������ѡͨ����ʧ���Ĵ����С�ִ�����ƽУ׼ʱ��ֻ��ѡ��һ��ͨ����
         0110 �ڲ�������(����)У׼  -���ڸ�У׼�������������ѹ���Զ����ӵ�ѡ����ģ�����롣 'RDY��У׼����ʱ��Ϊ�ߵ�ƽ����У׼���ʱ���ص͵�ƽ��У׼������ɺ�ADC���ڿ���ģʽ����õ�������ϵ����������ѡͨ��������Ĵ����С�ÿ�θ���һ��ͨ��������ʱ������Ҫִ��������У׼��ʹ�����������С��ִ��������У׼ʱ��ֻ��ѡ��һ��ͨ����  ��ȫ����ģʽ�£��޷�ִ���ڲ�������У׼��
         0111 ϵͳ���ƽ(ʧ��)У׼  -��ϵͳ���ƽ�������ӵ���ѡͨ����ͨ����������
         1000 ϵͳ������(����)У׼  -��ϵͳ�������������ӵ���ѡͨ����������
    */
#define AD7124_ADC_CTRL_REG_CLK_SEL(x)    (((x) & 0x3) << 0)    //ѡ��ADC��ʱ��Դ Ƭ�ڻ����ⲿʱ��

/* IO_Control_1 Register bits -  IO_CONTROL_1�Ĵ��� */
#define AD7124_IO_CTRL1_REG_GPIO_DAT2     (1 << 23)     //�������P2��GPIO_CTRL2��1ʱ�� GPIO_DAT2λ����ͨ���������P2��ֵ��
#define AD7124_IO_CTRL1_REG_GPIO_DAT1     (1 << 22)     //�������P1��GPIO_CTRL1��1ʱ�� GPIO_DAT1λ����ͨ���������P1��ֵ��
#define AD7124_IO_CTRL1_REG_GPIO_CTRL2    (1 << 19)     //�������P2ʹ�ܡ�GPIO_CTRL2��1ʱ���������P2��Ч��
#define AD7124_IO_CTRL1_REG_GPIO_CTRL1    (1 << 18)     //�������P1ʹ�ܡ�GPIO_CTRL1��1ʱ���������P1��Ч��
#define AD7124_IO_CTRL1_REG_PDSW          (1 << 15)     //���ŹضϿ��ؿ���λ����λ��1ʱ�����ŹضϿ���PDSW�պ�(��AGND����)
#define AD7124_IO_CTRL1_REG_IOUT1(x)      (((x) & 0x7) << 11)   //IOUT1����������ֵ��100 = 500 ��A  101 = 750 ��A  110 = 1000 ��A��
#define AD7124_IO_CTRL1_REG_IOUT0(x)      (((x) & 0x7) << 8)    //IOUT0����������ֵ
#define AD7124_IO_CTRL1_REG_IOUT_CH1(x)   (((x) & 0xF) << 4)    //IOUT1����������ͨ��ѡ��λ 0000-1111
#define AD7124_IO_CTRL1_REG_IOUT_CH0(x)   (((x) & 0xF) << 0)    //IOUT0����������ͨ��ѡ��λ


/* IO_Control_2 Register bits */
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS7   (1 << 15) //ʹ��AIN7ͨ���ϵ�ƫ�õ�ѹ����1ʱ���ڲ�ƫ�õ�ѹͨ��AIN7�ṩ��
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS6   (1 << 14)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS5   (1 << 11)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS4   (1 << 10)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS3   (1 << 5)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS2   (1 << 4)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS1   (1 << 1)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS0   (1 << 0)


/* ID Register bits - ID */
#define AD7124_ID_REG_DEVICE_ID(x)   (((x) & 0xF) << 4) 
#define AD7124_ID_REG_SILICON_REV(x) (((x) & 0xF) << 0)

/* Error Register bits - ����Ĵ��� */
#define AD7124_ERR_REG_LDO_CAP_ERR        (1 << 19) //ģ��/����LDOȥ����ݼ�顣���ģ�������LDO��Ҫ��ȥ�����δ���ӵ�AD7124-8���˱�־λ��1��
#define AD7124_ERR_REG_ADC_CAL_ERR        (1 << 18) //У׼��顣���У׼��������δ��ɣ��˱�־λ��1��ʾУ׼��������
#define AD7124_ERR_REG_ADC_CONV_ERR       (1 << 17) //��λ��ʾת������Ƿ���Ч�����ת�������з������󣬴˱�־λ��1��
#define AD7124_ERR_REG_ADC_SAT_ERR        (1 << 16) //ADC���ͱ�־�����ת�������е��������ͣ��˱�־λ��1��
#define AD7124_ERR_REG_AINP_OV_ERR        (1 << 15) //AINP�ϵĹ�ѹ��⡣
#define AD7124_ERR_REG_AINP_UV_ERR        (1 << 14) //AINP�ϵ�Ƿѹ��⡣
#define AD7124_ERR_REG_AINM_OV_ERR        (1 << 13) //AINM�ϵĹ�ѹ��⡣
#define AD7124_ERR_REG_AINM_UV_ERR        (1 << 12) //AINM�ϵ�Ƿѹ��⡣
#define AD7124_ERR_REG_REF_DET_ERR        (1 << 11) //��׼��ѹ��⡣��ADC���õ��ⲿ��׼��ѹ��·��С��0.7 Vʱ���˱�־λ��1��
#define AD7124_ERR_REG_DLDO_PSM_ERR       (1 << 9)  //����LDO�����������LDO��⵽���󣬴˱�־λ��1��
#define AD7124_ERR_REG_ALDO_PSM_ERR       (1 << 7)  //ģ��LDO�������ģ��LDO��ѹ��⵽���󣬴˱�־λ��1��
#define AD7124_ERR_REG_SPI_IGNORE_ERR     (1 << 6)  //ִ���ڲ��Ĵ�����CRC���ʱ���޷�����Ƭ�ڼĴ�����
#define AD7124_ERR_REG_SPI_SLCK_CNT_ERR   (1 << 5)  //���д���ͨ�Ŷ���8λ��ĳһ��������SCLK����������8�ı���ʱ����λ��1��
#define AD7124_ERR_REG_SPI_READ_ERR       (1 << 4)  //SPI�������ڼ䷢������ʱ����λ��1
#define AD7124_ERR_REG_SPI_WRITE_ERR      (1 << 3)  //SPIд�����ڼ䷢������ʱ����λ��1��
#define AD7124_ERR_REG_SPI_CRC_ERR        (1 << 2)  //crc ����
#define AD7124_ERR_REG_MM_CRC_ERR         (1 << 1)  //�洢��ӳ�����ÿ��д��Ĵ���ʱ������Դ洢��ӳ��ִ��CRC����

/* Error_En Register bits  */
#define AD7124_ERREN_REG_MCLK_CNT_EN           (1 << 22)    //��ʱ�Ӽ���������λ��1ʱ����ʱ�Ӽ�����ʹ�ܣ�
#define AD7124_ERREN_REG_LDO_CAP_CHK_TEST_EN   (1 << 21)    //ģ��/����LDOȥ����ݼ��Ĳ��ԡ���λ��1ʱ��ȥ�������LDO���ڲ��Ͽ���ǿ�Ʋ������ϡ��������û��Ϳ��Բ���ģ�������LDOȥ����ݼ�����õĵ�·
#define AD7124_ERREN_REG_LDO_CAP_CHK(x)        (((x) & 0x3) << 19) //ģ��/����LDOȥ����ݼ�顣
#define AD7124_ERREN_REG_ADC_CAL_ERR_EN        (1 << 18) //��λ��1ʱ��У׼���ϼ��ʹ�ܡ�
#define AD7124_ERREN_REG_ADC_CONV_ERR_EN       (1 << 17) //��λ��1ʱ�����ת��������ת������ʱ��1
#define AD7124_ERREN_REG_ADC_SAT_ERR_EN        (1 << 16) //��λ��1ʱ��ADC���������ͼ��ʹ�ܡ�
#define AD7124_ERREN_REG_AINP_OV_ERR_EN        (1 << 15) //��λ��1ʱ������ʹ�ܵ�AINPͨ���ϵĹ�ѹ�����ʹ�ܡ�
#define AD7124_ERREN_REG_AINP_UV_ERR_EN        (1 << 14) //��λ��1ʱ������ʹ�ܵ�AINPͨ���ϵ�Ƿѹ�����ʹ�ܡ�
#define AD7124_ERREN_REG_AINM_OV_ERR_EN        (1 << 13) //��λ��1ʱ������ʹ�ܵ�AINMͨ���ϵĹ�ѹ�����ʹ��
#define AD7124_ERREN_REG_AINM_UV_ERR_EN        (1 << 12) //��λ��1ʱ������ʹ�ܵ�AINMͨ���ϵ�Ƿѹ�����ʹ�ܡ�
#define AD7124_ERREN_REG_REF_DET_ERR_EN        (1 << 11) //��λ��1ʱ���������ADCʹ�õ��ⲿ��׼��ѹԴ������ⲿ��׼��ѹԴ��·����ֵС��0.7 V�������־λ�ͻ���1��
#define AD7124_ERREN_REG_DLDO_PSM_TRIP_TEST_EN (1 << 10) //���������LDO�Ĳ��Ի��ơ���λ��1ʱ�����Ե�·���������ӵ�DGND��
#define AD7124_ERREN_REG_DLDO_PSM_ERR_ERR      (1 << 9)  //��λ��1ʱ�������������LDO��ѹ
#define AD7124_ERREN_REG_ALDO_PSM_TRIP_TEST_EN (1 << 8)  //�����ģ��LDO�Ĳ��Ի��ơ���λ��1ʱ�����Ե�·���������ӵ�AVSS��
#define AD7124_ERREN_REG_ALDO_PSM_ERR_EN       (1 << 7)  //��λ��1ʱ���������ģ��LDO��ѹ��
#define AD7124_ERREN_REG_SPI_IGNORE_ERR_EN     (1 << 6)  //ִ���ڲ��Ĵ�����CRC���ʱ���޷�����Ƭ�ڼĴ�����
#define AD7124_ERREN_REG_SPI_SCLK_CNT_ERR_EN   (1 << 5)  //��λ��1ʱ��SCLK������ʹ�ܡ�
#define AD7124_ERREN_REG_SPI_READ_ERR_EN       (1 << 4)  //��λ��1ʱ������������ڼ䷢�����󣬴���Ĵ����е�SPI_READ_ERRλ�ͻ���1
#define AD7124_ERREN_REG_SPI_WRITE_ERR_EN      (1 << 3)  //��λ��1ʱ�����д�����ڼ䷢�����󣬴���Ĵ����е�SPI_WRITE_ERRλ�ͻ���1��
#define AD7124_ERREN_REG_SPI_CRC_ERR_EN        (1 << 2)  //��λʹ�ܶ����ж�д������CRC��顣
#define AD7124_ERREN_REG_MM_CRC_ERR_EN         (1 << 1)  //�����λ��1����ÿ��д��Ĵ���ʱ������Դ洢��ӳ��ִ��CRC���㡣

/* Channel Registers 0-15 bits - ͨ���Ĵ��� */
#define AD7124_CH_MAP_REG_CH_ENABLE    (1 << 15)    //ͨ��ʹ��λ����λ��1��ʹ������ͨ������ת�����С�Ĭ������£���ͨ��0��Enableλ��1��
#define AD7124_CH_MAP_REG_CH_DISABLE   (0 << 15)    //ͨ��ʧ��λ
#define AD7124_CH_MAP_REG_SETUP(x)     (((x) & 0x7) << 12)  //����ѡ����Щλ������ͨ��ʹ��8�������е���һ��������ADC��
#define AD7124_CH_MAP_REG_AINP(x)      (((x) & 0x1F) << 5)  //��ģ������AINP����ѡ��
#define AD7124_CH_MAP_REG_AINM(x)      (((x) & 0x1F) << 0)  //��ģ������AINM����ѡ��

/* Configuration Registers 0-7 bits ���üĴ���  ---- app setup */
#define AD7124_CFG_REG_BIPOLAR     (1 << 11)    //����ѡ��λ����λ��1ʱ��ѡ��˫���Թ���ģʽ��
#define AD7124_CFG_REG_UNIPOLAR	   (0 << 11)    //������
#define AD7124_CFG_REG_BURNOUT(x)  (((x) & 0x3) << 9)   //��Щλѡ�񴫸�����·������Դ�ķ��ȡ�
#define AD7124_CFG_REG_REF_BUFP    (1 << 8) //REFINx(+)�ϵĻ�����ʹ�ܡ���λ��1ʱ����������׼��ѹ����(�ڲ����ⲿ)
#define AD7124_CFG_REG_REF_BUFM    (1 << 7) //REFINx(?)�ϵĻ�����ʹ�ܡ���λ��1ʱ�����帺��׼��ѹ����(�ڲ����ⲿ)
#define AD7124_CFG_REG_AIN_BUFP    (1 << 6) //AINP�ϵĻ�����ʹ�ܡ���λ��1ʱ��������ѡ����ģ���������š�
#define AD7124_CFG_REG_AINN_BUFM   (1 << 5) //AINM�ϵĻ�����ʹ�ܡ���λ��1ʱ��������ѡ�ĸ�ģ���������š�
#define AD7124_CFG_REG_REF_SEL(x)  ((x) & 0x3) << 3 
/*
    ��׼��ѹԴѡ��λ�����ô����üĴ���ת���κ�ͨ��ʱ����Щλѡ��Ҫʹ�õĻ�׼��ѹԴ��
    00 REFIN1(+)/REFIN1(-)
    01 REFIN2(+)/REFIN2(-)
    10 = �ڲ���׼��ѹԴ��
    11 = AVDD��
*/
#define AD7124_CFG_REG_PGA(x)      (((x) & 0x7) << 0)   //����ѡ��λ


/* Filter Register 0-7 bits - �˲��Ĵ��� */
#define AD7124_FILT_REG_FILTER(x)         (((x) & 0x7) << 21)   //�˲�������ѡ��λ����Щλѡ���˲������͡�
#define AD7124_FILT_REG_REJ60             (1 << 20)             //��λ��1ʱ�����sinc�˲����ĵ�һ�ݲ�Ƶ��Ϊ50 Hz����һ���ݲ�Ƶ�ʱ�����60 Hz���Ӷ�ʵ��50 Hz��60 Hzͬʱ���ơ�
#define AD7124_FILT_REG_POST_FILTER(x)    (((x) & 0x7) << 17)   //�����˲�������ѡ��λ����Щ�˲���λ��1ʱ��sinc3���һ�������˲�����
#define AD7124_FILT_REG_SINGLE_CYCLE      (1 << 16)             //������ת��ʹ��λ��
#define AD7124_FILT_REG_FS(x)             (((x) & 0x7FF) << 0)  //�˲��������������ѡ��λ��

/******************************************************************************/
/************* Enums and structures that define the AD7124 device *************/
/******************************************************************************/

/*! Device register info */
typedef struct _ad7124_st_reg
{
	int32_t addr;
	int32_t value;
	int32_t size;
	int32_t rw;
}ad7124_st_reg;

typedef struct{
    float temperature;  //�¶�
    float pressure;     //ѹ��
}SensorData_t;
extern SensorData_t nowData;
extern SensorData_t lastData;

extern float kaltemp_min;
extern float kaltemp_max;
extern float kalpress_min;
extern float kalpress_max;
extern float kalmanoutput_t;
extern float kalmanoutput_p;
extern uint8_t adckalman_flag;




#define MAX_DEVIATION_P     20
#define MAX_DEVIATION_T     0.2

/*! AD7124 registers list*/
enum ad7124_registers
{
	AD7124_Status = 0x00,
	AD7124_ADC_Control,
	AD7124_Data,
	AD7124_IOCon1,
	AD7124_IOCon2_,
	AD7124_ID,
	AD7124_Error,
	AD7124_Error_En,
	AD7124_Mclk_Count,
	AD7124_Channel_0,
	AD7124_Channel_1,
	AD7124_Channel_2,
	AD7124_Channel_3,
	AD7124_Channel_4,
	AD7124_Channel_5,
	AD7124_Channel_6,
	AD7124_Channel_7,
	AD7124_Channel_8,
	AD7124_Channel_9,
	AD7124_Channel_10,
	AD7124_Channel_11,
	AD7124_Channel_12,
	AD7124_Channel_13,
	AD7124_Channel_14,
	AD7124_Channel_15,
	AD7124_Config_0,
	AD7124_Config_1,
	AD7124_Config_2,
	AD7124_Config_3,
	AD7124_Config_4,
	AD7124_Config_5,
	AD7124_Config_6,
	AD7124_Config_7,
	AD7124_Filter_0,
	AD7124_Filter_1,
	AD7124_Filter_2,
	AD7124_Filter_3,
	AD7124_Filter_4,
	AD7124_Filter_5,
	AD7124_Filter_6,
	AD7124_Filter_7,
	AD7124_Offset_0,
	AD7124_Offset_1,
	AD7124_Offset_2,
	AD7124_Offset_3,
	AD7124_Offset_4,
	AD7124_Offset_5,
	AD7124_Offset_6,
	AD7124_Offset_7,
	AD7124_Gain_0,
	AD7124_Gain_1,
	AD7124_Gain_2,
	AD7124_Gain_3,
	AD7124_Gain_4,
	AD7124_Gain_5,
	AD7124_Gain_6,
	AD7124_Gain_7,
	AD7124_REG_NO
};

extern uint8_t AD_ID1_REG;		//��λֵΪ0x12��0x14
extern volatile uint8_t DATA_STATUS;	//��ȡ��ͨ����
extern uint8_t AD_Gain;
extern volatile float PT100_TEMP;
extern volatile float PA;

void AD7124_Reset(void);
void AD7124_Init(void);
uint8_t AD7124_SPI_ReadWrite(uint8_t Data);
void AD7124_MUL_INIT(uint8_t SamHz);
uint32_t AD7124_Read_Data(uint8_t byte);	//��ȡ�������������uint32_t ���ͽ��
//uint8_t AD7124_Read_Byte(void);		//�ֽڶ�ȡ����
uint32_t AD7124_Read_Reg(uint8_t addr,uint8_t byte);//��ȡ�Ĵ�����ַ
uint16_t AD7124_Write_Reg(uint8_t addr,uint8_t byte,uint32_t data);
void AD7124_Set_Gain(uint8_t gain);
uint16_t Get_AD7124_ID(void);//��ȡID ��֤ͨѶ
float AD7124_READ_DATAREG(void);    //��ȡ���ݼĴ���
void read_reg(void);	//��ȡ���üĴ�����ģʽ�Ĵ���ֵ���������ȫ�ֱ���conf_reg[3]��mode_reg[3]��
float AD7124_First_Filter(float *GetVol);
void AD7124_PT100_DATA(void);
void AD7124_DATA(void);
SensorData_t limitFilter(SensorData_t newData); 
extern void Filter_func(void);

#endif









