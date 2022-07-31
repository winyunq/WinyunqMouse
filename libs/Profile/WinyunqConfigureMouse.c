/********************************** (C) COPYRIGHT *******************************
 * File Name          : battservice.c
 * Author             : Winyunq
 * Version            : V1.0
 * Date               : 2021/8/28
 * Description        : 配置软件

 *******************************************************************************/
#include "CONFIG.h"
#include "ConfigureWinyunqMouse.h"
#include "WinyunqMouse.h"
const uint8 configureserver[ATT_UUID_SIZE] =
{
 'W','i','n','y','u','n','q',' ','H','I','D','M','o','u','s','e'
};

// Characteristic 1 UUID: 0xFFF1
const uint8 configurechar[ATT_UUID_SIZE] =
{
 'W','i','n','y','u','n','q',' ','H','I','D','M','o','u','s','e'
};
static gattCharCfg_t mouseconfigure[GATT_MAX_NUM_CONN];
static const gattAttrType_t mouseprofile = { ATT_UUID_SIZE, configureserver };
static uint8 priv= GATT_PROP_READ|GATT_PROP_WRITE|GATT_PROP_WRITE_NO_RSP ;

static uint8 mousedatabuffer[MouseConfigureSize] = { 0 };

static gattAttribute_t mousedatafile[] =
{
 {
  { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
  GATT_PERMIT_READ,                         /* permissions */
  0,                                        /* handle */
  (uint8 *)&mouseprofile            /* pValue */
 },
 {
  { ATT_BT_UUID_SIZE, characterUUID },
  GATT_PERMIT_READ,
  0,
  &priv
 },
 {
  { ATT_UUID_SIZE, configurechar },
  GATT_PERMIT_READ|GATT_PERMIT_WRITE|GATT_PERMIT_ENCRYPT_WRITE,
  0,
  mousedatabuffer
 },/*
  {
    { ATT_BT_UUID_SIZE, clientCharCfgUUID },
    GATT_PERMIT_READ | GATT_PERMIT_WRITE,
    0,
    (uint8 *)WinyunqMouseConfigureProfileConfig
  },
  {
    { ATT_BT_UUID_SIZE, charUserDescUUID },
    GATT_PERMIT_READ,
    0,
    WinyunqMouseConfigureDescripe
  },*/
};

static bStatus_t providetohost( uint16 connHandle, gattAttribute_t *pAttr,
                                                     uint8 *pValue, uint16 *pLen, uint16 offset, uint16 maxLen,uint8 method );
static bStatus_t getdata( uint16 connHandle, gattAttribute_t *pAttr,
                                                   uint8 *pValue, uint16 len, uint16 offset,uint8 method );

gattServiceCBs_t WinyunqMouseConfigureProfileCBs =
{
 providetohost,  // Read callback function pointer
 getdata, // Write callback function pointer
 NULL                       // Authorization callback function pointer
};

bStatus_t WinyunqMouseConfigure_AddService()
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, mouseconfigure );
  // Register GATT attribute list and CBs with GATT Server App
  status = GATTServApp_RegisterService( mousedatafile,
                                        GATT_NUM_ATTRS( mousedatafile ),
                                        GATT_MAX_ENCRYPT_KEY_SIZE,
                                        &WinyunqMouseConfigureProfileCBs );
  return ( status );
}
static bStatus_t providetohost( uint16 connHandle, gattAttribute_t *pAttr,
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

static bStatus_t getdata( uint16 connHandle, gattAttribute_t *pAttr,
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
      GATTServApp_InitCharCfg( connHandle, mouseconfigure );
    }
  }
}

void ConfigureMouse(char*buffer){
  extern uint32 UsingSleepTime;
  UsingSleepTime=0;//该操作将阻止休眠
  tmos_memcpy(MouseConfigure.data,buffer, MouseConfigureSize );//
  MouseConfigure.data[0]&=ConfigureSetting;//仅有权限允许位能绑定成功
    DefaultLED(0,0);
  EEPROM_WRITE(0, MouseConfigure.data, MouseConfigureSize );
}
