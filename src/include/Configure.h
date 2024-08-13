/// 设备名称
#define WinyunqMouseIDName -28, -70, -111, -27, -72, -99, -28, -71, -117, -26, -116, -121
/// 设备名称长度
#define WinyunqMouseIDNameLength 12
//产品名称,格式 云指 版本代号 续航代号 尺寸代号 体积代号,如云指1A
//指尖鼠标
//#define WinyunqMouseIDName -26,-116,-121,-27,-80,-106,-23,-68,-96,-26,-96,-121
//#define WinyunqMouseIDNameLength 12
//云指1AML
//#define WinyunqMouseIDName -28, -70, -111, -26, -116, -121,'1','C','M','R'
//#define WinyunqMouseIDNameLength 10
//#define WinyunqMouseIDName -28, -70, -111, -26, -116, -121,'1','A'
//云指     -28, -70, -111, -26, -116, -121
//指尖鼠标-26,-116,-121,-27,-80,-106,-23,-68,-96,-26,-96,-121
/// GPIO配置
#define LeftTouch GPIO_Pin_22
#define MoveUP GPIO_Pin_13
#define MoveDown GPIO_Pin_14
#define MoveLeft GPIO_Pin_15
#define MoveRight GPIO_Pin_7
#define MoveUPDown MoveUP|MoveDown
#define MoveLeftRight MoveLeft|MoveRight
#define MoveGroup MoveUP|MoveDown|MoveLeft|MoveRight
//#define LEDIndicator GPIO_Pin_9
#define LEDIndicator GPIO_Pin_13
/// 采用PWM控制灯的亮度，启用该宏则采用PWM控制灯的开关，屏蔽则禁用之
#define LEDPWMChannel CH_PWM5
#define LEDOpenLevel 0
#define power GPIO_Pin_12
#define PoweGPIOChannel CH_EXTIN_2
/// 是否开启USB功能。为了让用户体验产品，而不是去适应产品，默认情况下不应该开启此功能。
//#define UsingUSB 1
/// 是否开启蓝牙功能。此选型是针对设计有失误，霍尔被蓝牙干扰的情况下迫不得已设计的。
#define UsingBLE 1
/// 是否使用检测上升下降沿的霍尔
#define UsingUPDowmHallEdge 1
/// 是否允许设备休眠
//#define UsingSleep 1
/*CH57X
  CH_EXTIN_0 = 0,     // ADC 外部模拟通道 0    GPIOA4
  CH_EXTIN_1,       // ADC 外部模拟通道 1      GPIOA5
  CH_EXTIN_2,       // ADC 外部模拟通道 2      GPIOA12
  CH_EXTIN_3,       // ADC 外部模拟通道 3      GPIOA13
  CH_EXTIN_4,       // ADC 外部模拟通道 4      GPIOA14
  CH_EXTIN_5,       // ADC 外部模拟通道 5      GPIOA15
  CH_EXTIN_8 = 8,     // ADC 外部模拟通道 8    GPIOA4
  CH_EXTIN_9,       // ADC 外部模拟通道 9      GPIOA5
  CH_EXTIN_12 = 12,   // ADC 外部模拟通道 12   GPIOA14
  CH_EXTIN_13,      // ADC 外部模拟通道 13     GPIOA9
*/
/// 鼠标移动所需霍尔元器件的供能GPIOB
#define trackballpower  GPIO_Pin_4
/// 时间系设定
/// 秒设定，单位时间0.625ms，一秒为1600
#define Second( time ) time*1600
/// 设备无操作休眠时间设定
#define WinyunqMouseSleepTime  MouseConfigure.details.sleep*5*2000
/// 移动模式下，当鼠标速度过快时，会进入高速移动模式，此参数用于设定高速阈值
#define WinyunqMouseMoveSpeedListTime MouseConfigure.details.speed*5*20

/// 等待连接指示灯闪烁频率
#define WaitConnectIntervalLED (Second(1)/2)
///
#define OTAProgramIntervalLED (Second(0.5)/2)
#define MouseConfigureSize 8
//支持权限设定
#define ConfigureSetting ( SupportLeftRightTouch|SupportTrackBall|SupportWorkAsHelp|SupportLED|SUpportMoveTypeBySpeed )
#define SupportLeftRightTouch 0x1
#define SupportTrackBall 0x2
#define SupportWorkAsHelp 0x4
#define SupportLED 0x8
#define SUpportMoveTypeBySpeed 0x10
/// 关掉的时钟
#define CloseClock RB_SLP_CLK_TMR0|RB_SLP_CLK_TMR1|RB_SLP_CLK_TMR2|RB_SLP_CLK_TMR3|RB_SLP_CLK_UART0|RB_SLP_CLK_UART1|RB_SLP_CLK_UART2|RB_SLP_CLK_UART3|RB_SLP_CLK_SPI0|\
RB_SLP_CLK_PWMX|RB_SLP_CLK_USB|RB_SLP_USB_WAKE|RB_SLP_RTC_WAKE

/// 数据类型定义

typedef char boolean;
typedef unsigned char uint8;
typedef signed short int16;
typedef signed long int32;
typedef unsigned long UINT32;

#ifndef BOOL
typedef unsigned char                   BOOL;
#endif
#ifndef s8
typedef signed char                     s8;
#endif
#ifndef s16
typedef signed short                    s16;
#endif
#ifndef s32
typedef signed long                     s32;
#endif
#ifndef u8
typedef unsigned char                   u8;
#endif
#ifndef u16
typedef unsigned short                  u16;
#endif
#ifndef u32
typedef unsigned long                   u32;
#endif
#ifndef u64
typedef unsigned long long              u64;
#endif
#ifndef u8C
typedef const unsigned char             u8C;
#endif
#ifndef u32V
typedef unsigned long volatile          u32V;
#endif
#ifndef int8
typedef signed char                     int8;
#endif
#ifndef int16
typedef short                           int16;
#endif
#ifndef int32
typedef signed long                     int32;
#endif
#ifndef uint8
typedef unsigned char                   uint8;
#endif
#ifndef uint16
typedef unsigned short                  uint16;
#endif
#ifndef uint32
typedef unsigned long                   uint32;
#endif
#ifndef int8_t
typedef signed char                     int8_t;
#endif
#ifndef int16_t
typedef short                           int16_t;
#endif
#ifndef int32_t
typedef signed long                     int32_t;
#endif
#ifndef uint8_t
typedef unsigned char                   uint8_t;
#endif
#ifndef uint16_t
typedef unsigned short                  uint16_t;
#endif
#ifndef uint32_t
typedef unsigned long                   uint32_t;
#endif
