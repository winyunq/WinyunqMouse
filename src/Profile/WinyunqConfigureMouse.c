/**
 * @file            文件名:                WinyunqConfigureMouse.c
 * @brief           蓝牙服务-“配置”文件                                      
 *  @details        为了实现设备在兼容过去的同时又面向未来，需要一种“切换工作方式”的功能，记录当前设备的使用方法，因此，需要一个“配置”功能
 *                  可以由用户自定义修改
 *                  具体使用方法：在合理的地方(如官方模板中的HidDev_Init()处)调用一次AddServiceWinyunqMouseConfigure();完成“配置”的蓝牙服务注册即可
 * 
 * @author          Winyunq             创建
 * @version         版本:                 1.0.0
 * @date            2022-10-15          创建
 * 
 * Copyright (c) 2022  Winyunq科技公司
 */
#define __WinyunqConfigure_C
#include "CONFIG.h"
#include "WinyunqConfigure.h"

/// “配置”服务的UUID，为了使UUID有意义，在此采用字符串强转16字节数据
const uint8 WinyunqMouseConfigureProfileServUUID[ATT_UUID_SIZE] =
{
 'W','i','n','y','u','n','q',' ','H','I','D','M','o','u','s','e'
};

/// ”配置“服务的特征值UUID，隶属于服务UUID下的子UUID，为了使UUID有意义，在此采用字符串强转16字节数据
const uint8 WinyunqMouseConfigureProfilecharUUID[ATT_UUID_SIZE] =
{
 'W','i','n','y','u','n','q',' ','H','I','D','M','o','u','s','e'
};
/// “配置”
static gattCharCfg_t WinyunqMouseConfigureProfileConfig[GATT_MAX_NUM_CONN];
/// ”配置“服务
static const gattAttrType_t WinyunqMouseConfigureProfileService = { ATT_UUID_SIZE, WinyunqMouseConfigureProfileServUUID };
/// 权限
static uint8 WinyunqMouseConfigureProfileCharProps= GATT_PROP_READ|GATT_PROP_WRITE|GATT_PROP_WRITE_NO_RSP ;
/// 服务缓冲区
static uint8 WinyunqMouseConfigureServerBuffer[MouseConfigureSize] = { 0 };
/// 蓝牙服务特征值表
static gattAttribute_t WinyunqMouseConfigureProfile[] =
{
 {
  { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
  GATT_PERMIT_READ,                         /* permissions */
  0,                                        /* handle */
  (uint8 *)&WinyunqMouseConfigureProfileService            /* pValue */
 },
 {
  { ATT_BT_UUID_SIZE, characterUUID },
  GATT_PERMIT_READ,
  0,
  &WinyunqMouseConfigureProfileCharProps
 },
 {
  { ATT_UUID_SIZE, WinyunqMouseConfigureProfilecharUUID },
  GATT_PERMIT_READ|GATT_PERMIT_WRITE|GATT_PERMIT_ENCRYPT_WRITE,
  0,
  WinyunqMouseConfigureServerBuffer
 },
};

static bStatus_t WinyunqMouseProvideMessageToHostCB( uint16 connHandle, gattAttribute_t *pAttr,
                                                     uint8 *pValue, uint16 *pLen, uint16 offset, uint16 maxLen,uint8 method );
static bStatus_t WinyunqMouseGetMessageFromHostCB( uint16 connHandle, gattAttribute_t *pAttr,
                                                   uint8 *pValue, uint16 len, uint16 offset,uint8 method );
/// 配置服务回调函数组
gattServiceCBs_t WinyunqMouseConfigureProfileCBs =
{
 WinyunqMouseProvideMessageToHostCB,  // Read callback function pointer
 WinyunqMouseGetMessageFromHostCB, // Write callback function pointer
 NULL                       // Authorization callback function pointer
};
/**
 * @brief           蓝牙服务-"配置"的注册函数                                      
 *  @details        在适当的位置调用一次该函数，注册相关服务。基于WCH官方例程，应该在HidDev_Init()处调用
 * 
 * 
 * @return          bStatus_t类型         是否成功注册蓝牙特征值                                                         
 *  @retval         SUCCESS(0)            成功
 *  @retval         其它(非0)             失败
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-10-14
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-10-14
 * @version         1.0.0
 */
bStatus_t AddServiceWinyunqMouseConfigure()
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, WinyunqMouseConfigureProfileConfig );
  // Register GATT attribute list and CBs with GATT Server App
  status = GATTServApp_RegisterService( WinyunqMouseConfigureProfile,
                                        GATT_NUM_ATTRS( WinyunqMouseConfigureProfile ),
                                        GATT_MAX_ENCRYPT_KEY_SIZE,
                                        &WinyunqMouseConfigureProfileCBs );
  return ( status );
}
/**
 * @brief           向主机提供数据的回调函数                                      
 *  @details        主机发起特征值访问，查询“配置”参数。系统调用该函数完成提供所需要的参数（用户自行编写该函数）
 * 
 * @param           参数名称:【connHandle】   数据类型:uint16       连接句柄
 * @param           参数名称:【pAttr】        数据类型:gattAttribute_t*  GATT地址
 * @param           参数名称:【pValue】       数据类型:uint8*       需要提供给主机的数据区
 * @param           参数名称:【pLen】         数据类型:uint16*      提供给主机的数据区长度
 * @param           参数名称:【offset】       数据类型:uint16       
 * @param           参数名称:【maxLen】       数据类型:maxLen       最大长度
 * @param           参数名称:【method】       数据类型:uint8        方法类型
 * 
 * @return          bStatus_t类型         返回值介绍                                                         
 *  @retval         SUCCESS(0)            成功
 *  @retval         其它(非0)             失败
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-10-15
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-10-15
 * @version         1.0.0
 */
static bStatus_t WinyunqMouseProvideMessageToHostCB( uint16 connHandle, gattAttribute_t *pAttr,
                                                     uint8 *pValue, uint16 *pLen, uint16 offset, uint16 maxLen,uint8 method  )
{
  bStatus_t status = SUCCESS;

  // If attribute permissions require authorization to read, return error
  if ( gattPermitAuthorRead( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
  *pLen = MouseConfigureSize;
  tmos_memcpy( pValue,MouseConfigure.data, MouseConfigureSize );
  return ( status );
}
/**
 * @brief           接收来自主机消息的回调函数                                      
 *  @details        主机向“配置”服务发送数据时，系统自动调用该函数处理接收消息（用户自行编写该函数）
 * 
 * @param           参数名称:【connHandle】   数据类型:uint16       连接句柄
 * @param           参数名称:【pAttr】        数据类型:gattAttribute_t*  GATT地址
 * @param           参数名称:【pValue】       数据类型:uint8*       接收来自主机数据的数据区
 * @param           参数名称:【pLen】         数据类型:uint16       数据区里数据的长度
 * @param           参数名称:【offset】       数据类型:uint16       
 * @param           参数名称:【method】       数据类型:uint8        方法类型
 * 
 * @return          bStatus_t类型         返回值介绍                                                         
 *  @retval         SUCCESS(0)            成功
 *  @retval         其它(非0)             失败
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-10-14
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-10-14
 * @version         1.0.0
 */
static bStatus_t WinyunqMouseGetMessageFromHostCB( uint16 connHandle, gattAttribute_t *pAttr,
                                                   uint8 *pValue, uint16 len, uint16 offset,uint8 method  )
{
  bStatus_t status = SUCCESS;

  // If attribute permissions require authorization to write, return error
  if ( gattPermitAuthorWrite( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  if(len==MouseConfigureSize){
  if(pValue[MouseConfigureSize-1]==17)
    ConfigureMouse(pValue);
  }
  return ( status );
}
/**
 * @brief           句柄连接状态                                      
 *  @details        蓝牙通信状态发生改变时会调用该函数（可无视）
 * 
 * @param           参数名称:【connHandle】   数据类型:uint16      句柄ID
 * @param           参数名称:【changeType】   数据类型:uint8       改变的类型
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-10-14
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-10-14
 * @version         1.0.0
 */
void WinyunqMouse_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) &&
           ( !linkDB_Up( connHandle ) ) ) )
    {
      GATTServApp_InitCharCfg( connHandle, WinyunqMouseConfigureProfileConfig );
    }
  }
}
/**
 * @brief           配置鼠标参数                                      
 *  @details        调用该函数配置鼠标的可配置参数。可以由用户自行调用。接收到来自主机的配置信息时也会调用该函数配置鼠标参数
 * 
 * @param           参数名称:【buffer】       数据类型:char*        用于配置参数的二进制数据区
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-10-14
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-10-14
 * @version         1.0.0
 */
void ConfigureMouse(char* buffer){
  extern uint32 UsingSleepTime;
  UsingSleepTime=0;//�ò�������ֹ����
  tmos_memcpy(MouseConfigure.data,buffer, MouseConfigureSize );//
  MouseConfigure.data[0]&=ConfigureSetting;//仅有权限允许位能绑定成功
    DefaultLED(0,0);
  EEPROM_WRITE(0, MouseConfigure.data, MouseConfigureSize );
}
