/*
 * iot_system.h
 *
 *  Created on: 2021年12月1日
 *      Author: Administrator
 */
#ifndef COMPONENTS_INCLUDES_IOT_SYSTEM_H_
#define COMPONENTS_INCLUDES_IOT_SYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Online   1
#define Offline  0

#define DEBUG_HEAPSIZE   0
#define DEBUG_UART		 0
#define DEBUG_BLE		 1
#define DEBUG_INV        0
#define DEBUG_TEST 		 0
#define DEBUG_MQTT		 1
#define test_wifi	   	 1     //0 便携式  //1 wifi-X2
//#define Kennedy_System
//#define Kennedy_main


#define DEBUG_EN         1    //打印数据开关

#if DEBUG_EN

#define IOT_Printf(format, ... )   do { printf(format, ##__VA_ARGS__);}while(0);

#else
#define IOT_Printf(format, ... )   do { (void)0; }while(0);
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#if test_wifi
#define LOCAL_PROTOCOL_6_EN   1     //本地通讯协议 6.0 使能
#else
#define LOCAL_PROTOCOL_6_EN   0     //本地通讯协议 6.0 失能
#endif
/* enum definitions */
typedef enum {DISABLE = 0, ENABLE = !DISABLE} EventStatus,ControlStatus ;


#define USER_DATA_ADDR   288		//用户自定义数据偏移位置 sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)
#define USER_DATA_LEN    20		    //用户自定义数据长度



#if test_wifi
#define DATALOG_TYPE "34"          // 13号寄存器，采集器设备类型  11 wifi采集器  33 便携式电源 34 wifi-x2
#define DATALOG_SVERSION "3.2.0.4" // 21号寄存器，采集器软件版本
#else
#define DATALOG_TYPE "33"          // 13号寄存器，采集器设备类型  11 wifi采集器  33 便携式电源  34 wifi-x2
#define DATALOG_SVERSION "3.3.0.1" // 21号寄存器，采集器软件版本
#endif


#define AES_KEY_128BIT  "growatt_aes16key"

/***************************************************************
  *     数据存储地址相关宏定义
****************************************************************/
/* 第0扇区 */
#define RW_SIGN_ADDR		0x00				//读写标示，用于第一次下载完程序后的初始化操作
#define RW_SIGN_ADDR1		0x0A				//读写标示1, 是否初始化70号之后的参数
#define RW_SIGN_ADDR2		0x14				//读写标示1, 是否初始化81号之后的参数
#define RESEAVE_ADDR		0x100				//预留

/* 第1扇区 */
#define REG_LEN_LIST_ADDR	0x1000				//256byte,采集器各寄存器数据长度记录表，用来记录每个寄存器数值的长度
#define CONFIG_ADDR			0x1100				//3800byte采集器各寄存器数据存储
#define CONFIG_CRC16_ADDR	0x1FF1				//CRC校验存储地址,校验前3800byte

/* 第2扇区 */
#define BACKUP_CONFIG_ADDR	0x2000				//作为第一扇区的备份扇区

/* 第3扇区 */
#define RESEAVE_ADDR1		0x3000				//用作标示区
#define RECORD_FULL			0x3000				//记录区是否已满,一个字节
#define FULL				0xaa				//满标示符

/* 第4扇区 */
#define DATA_ADDR			0x4000				//离线数据保存区4080KBYTE
/* 第1028扇区 */
#define IAP_CACHE_ADDR		0x400000			//IAP数据缓存区,4Mbyte后


/* 新参数（升级文件下载信息记录）的存储地址*/
#define SaveNewParamsStart_Addr		0x606000	 // 占用一个扇区4096 = 4k

#define APPCRC32_ADDR	 		0x7FF000		 //APP文件CRC32存放地址
#define APPLENGTH_ADDR 		APPCRC32_ADDR+128	 //boot执行选择开关存放地址


 /****************************************************************
  *     数据存储相关宏定义
  ***************************************************************
  */

#define REG_LEN				50U							    //采集器每个参数长度，预留50byte
#define REGADDR(x)			(CONFIG_ADDR + (REG_LEN * x))	//寄存器地址计算
#define REG_NUM_MAX			70U								//采集器寄存器个数
#define BACKREGADDR(x)		BACKUP_CONFIG_ADDR + 0x100U + (REG_LEN * x)		//寄存器地址计算
#define PARAMETER_LENLIST_LEN	(CONFIG_ADDR - REG_LEN_LIST_ADDR)

#define DATA_SECTOR_MAX		1024			//数据存储区总扇区
#define DATA_SECTOR_START	4				//数据存储区起始扇区
#define DATA_SECTOR_END		1027			//数据存储区结束扇区

#define ITEM_SECTOR_MAX		16				//每条233，直接使用一个页	4096/255

#define CONFIG_CRC16_NUM	4056								//CRC16要校验的数据长度
#define CONFIG_LEN_MAX		REG_LEN*REG_NUM_MAX					//参数总长
#define CONFIG_CRC16_BUFADDR	4081			//USART1_RX_BUF 4096byte缓存中，CRC校验的位置
#define CONFIG_LEN_BUFADDR		0				//USART1_RX_BUF 4096byte缓存中，参数长度链表位置
#define CONFIG_BUFADDR			0x100			//USART1_RX_BUF 4096byte缓存中，参数的位置


/****************************************************************
  *     存储数据记录相关宏定义
  ***************************************************************
  */
#define	RECORD_SECTOR_ST			4U
#define	RECORD_SECTOR_END			1024U
#define RECORD_ADDR					RECORD_SECTOR_ST * SECTOR_SIZE
#define RECORD_ITEM_SIZE			256U				//占用大小
#define RECORD_ITEM_SIZE_TRUE		223U					//实际使用大小
#define RECORD_SECTOR_MAX_NUM		16U					//一个扇区最高存储条数

#define RECORD_AMOUNT_NUM			16000U			//最高存储条数
#define	RE_SECTOR_MAX	   			(RECORD_AMOUNT_NUM/RECORD_SECTOR_MAX_NUM)


/* 数据每页后面的标记位 */
#define RECORD_SVID_H				(223U)						//存储ID高位
#define RECORD_SVID_L				(224U)						//存储ID低位
#define RECORD_CRC_H				(225U)						//单笔CRC16高位
#define RECORD_CRC_L				(226U)						//单笔CRC16低位
#define RECORD_STA_SINGLE			(227U)						//单笔存储状态,是否已读已写
#define RECORD_PACK_ALL				(228U)						//跨页存储总包
#define RECORD_PACK_CUR				(229U)						//跨页存储当前包
#define RECORD_LAST_LEN				(230U)						//最后一页长度

#define RECORD_SEC_STA				255						//标记扇区的是否已读已写,也是占用了每页剩余的几个字节中的最后一个

//flash未写过为0xff，同一地址不擦除写，为0。(物理上的)
#define SEC_FULL	0x5AU			//扇区满标志
#define SEC_NULL	0xFFU			//扇区空标志
#define SEC_READ	0x00U			//扇区已读标志

#define PAGE_WR		0x5AU			//页已写标志
#define PAGE_NULL	0xFFU			//页已空标志
#define PAGE_RD		0x00U			//页已读标志

#define PAGE_SIGN_ADDR	10U				//页读写标志，用每页倒数第十个记录
#define SEC_SIGN_ADDR	1U			//扇区读写标志，用扇区倒数第1个记录

//逻辑上的
#define STATUS_SEC_FULL	0x5AU			//扇区满标志
#define STATUS_SEC_NULL	0xFFU			//扇区空标志
#define STATUS_SEC_READ	0xDDU			//扇区已读标志

#define SAVANUM_START	1111U
#define SAVANUM_END		61111U

/*station模式下连接的WiFi参数(热点名字/热点密码) */
#pragma pack(4)
typedef struct  _IOTsystem{
	uint8_t SysWorkMode;			 //设备轮训数据工作模式： 服务器模式   本地模式
	uint8_t BLEKeysflag;		     //蓝牙密钥校验标志位
	uint8_t BLEBindingGg;			 //蓝牙绑定设备G 1 绑定 ;g 0未绑定
	uint8_t Pv_state;				 //逆变器通讯

	volatile uint8_t Pv_Senderrflag; //发送命令码异常标志
	uint8_t UploadParam_Flag;   	 //主动上报0x19 TAB参数
	uint8_t Wifi_state;		    	 //wifi连接状态
	uint8_t Ble_state;		    	 //BLE连接状态

	uint8_t Mbedtls_state;			 //TCP连接状态	 //MQTT订阅状态  在mqtt.h
	uint8_t SSL_state;				 //SSL 初始化配置完成状态
	uint8_t Server_Online;			 //服务器通信正常在线状态
	uint8_t ServerACK_flag;			 //服务器应答标志
//	uint8_t ServerACK_DeathTime;	 //服务器应答死亡时间
	uint8_t ParamSET_ACKCode;		 //服务器设置 应答状态
	uint8_t SYS_RestartFlag;		 //采集器重启状态位
	uint8_t up14dateflag;			//波形诊断数据上传标志位
	uint8_t up00dateflah;			//保留补齐

	uint8_t upBMS03dateflag;		//BMS板数据 03数据上传
	uint8_t upBMS04dateflag;		//BMS板数据 04数据上传
	uint8_t upBDC03dateflag;		//BDC数据 03数据上传
	uint8_t upBDC04dateflag;		//BDC数据 04数据上传

	uint8_t Get_TabDataBUF[300];	 //要发送的数据
	uint8_t updatetimeflag;			 //到时间 上传04数据  标志位
    uint8_t upHistorytimeflag;		 // 到时间上传 0x50 历史数据 标志位
	uint8_t wifi_onoff;						//开关WIFI
	uint8_t ble_onoff;						//开关BLE
	uint8_t BLE_TimeoutOFFflag;				//关闭BLE 超时标志，自动关闭
	uint8_t BLE_ReadDone;                   //接收完成标志;
	uint8_t SmartConfig_OnOff_Flag;

	uint16_t ServerGET_REGNUM;		 		//服务器查询采集器寄存器个数
	uint16_t ServerGET_REGAddrStart ; 		//服务器查询采集器寄存器地址开始
	uint16_t ServerGET_REGAddrEnd ;	 		//服务器查询采集器寄存器地址结束
	uint16_t ServerGET_REGAddrBUFF[30]; 	//服务器查询采集器寄存器连续获取

	uint16_t ServerSET_REGNUM;				//服务器查询采集器寄存器个数
	uint16_t ServerSET_REGAddrStart; 		//服务器设置采集器寄存器地址开始
	uint16_t ServerSET_REGAddrLen;   		//服务器设置采集器寄存器数据长度

	uint16_t CID ;					 		//通讯编号
	uint16_t bCID_0x04  ;			 		//04指令通讯编号缓存
    uint16_t Up04dateTime ;			 		//采集器上传04间隔时间
    uint16_t Up03dateTime ;			 		//采集器上传03间隔时间

	uint16_t Parameter_length ;		 		//参数列表实际总长度
	uint16_t blemtu_value;					//BLE MTU 大小
	uint16_t Wifi_ActiveReset;				//手动设置重连wifi
	uint16_t WIFI_BLEdisplay;				//通知显示屏幕的状态	低位0-3 蓝牙 bit0  开1/关0   bit1  连接设备1/断开设备0高位4-7 wifi bit4  开1/关0   bit5  连接设备1/断开设备0

	unsigned int  uiPackageLen ;         	//升级包长度
	unsigned int  uiSumPackageNums ;     	//总升级包数
	unsigned int  uiCurrentPackageNums ; 	//当前升级包编号
	uint32_t Host_setflag;			 		//主机设置参数
	uint32_t SoftClockTick;					//软件时间  秒
	uint32_t BLEread_waitTime;				//等等接收超时
	uint32_t RSAEDSS;

	uint32_t uiSystemInitStatus;

} SYStem_state;


/***************************************
 * 便携式显示使用
 * */
#define Displayflag_BLEON       (uint16_t)0x0001
#define Displayflag_BLECON		(uint16_t)0x0002
#define Displayflag_SYSreset	(uint16_t)0x0004
#define Displayflag_WIFION	 	(uint16_t)0x0010
#define Displayflag_WIFICON		(uint16_t)0x0020

#define   Serverflag_NULL  		0x00    // NULL 状态
#define   Serverflag_disable    0x01      // 禁止保存IP和端口
#define   Serverflag_enable     0x02      // 允许设置IP和端口，但是需要网络连接成功后才能保存
#define   Serverflag_savetest   0x03      // 无条件允许设置和保存IP和端口，主要用于生产设置用
#define   Serverflag_backoldip   	0x04    // 备份正在连接的旧IP和端口
#define   Serverflag_savenowip   	0x05    // 无条件允许设置和保存IP和端口，主要用于设置IP成功连接网络后保存用
#define   Serverflag_GETflag   	0xff    // 获取设置参数状态

//typedef struct _TAB{
//char Language[4];
//char Protocol_Type[4];
//char Protocol_Version[4];
//char Backlight_Delay[4];
//char Data_Interval[4];
//char Inverter_RangeStart[4];
//char Inverter_RangeEnd[4];
//char Reservedx1[4];
//char DataloggerSN[50];
//char RollDelayTime[4];
//char UpdateDataloggerFirmware[4];
//}SysConfig;

enum SYSRegister{

	REG_language = 0,  	 	 //0     采集器界面语言  0:英文，1:中文
	REG_ProtocolType,        //1     数据采集器的协议类型,0：MODBUS_TCP,1：Internal
	REG_ProtocolVersion,     //2     数服协议”（即本协议）的版本号
	REG_BacklitDelay,        //3     ShinePano背光延时
	REG_Interval,        	 //4     数据采集器向网络服务器传送数据的间隔时间，单位为分钟
	REG_StartAddress,  	     //5     数据采集器对逆变器的起始搜索地址
	REG_EndAddress,     	 //6     数据采集器对逆变器的结束搜索地址
	REG_Reserved,  	     	 //7     预留参数,内部已使用，勿做其他用途
	REG_SN,  	     		 //8     数据采集器序列号
	REG_PollingTime,  	     //9     逆变器轮询时间延时
	REG_Updated,  	     	 //10    对数据采集器的固件进行更新的标识,0：正常工作,1：更新固件
	REG_FTP,  	     	     //11    wifi模块无法使用FTP,用于数据采集器进行自身固件及光伏设备固件更新的FTP设置（含用户名、密码、IP、端口，各参数间用井号分隔），数据格式为：UserName#Password#IP#Port#
	REG_DNS,  	     		 //12    数据采集器的DNS配置
	REG_CollectorType,  	 //13    数据采集器类型
	REG_ClientIP,  	     	 //14    数据采集器的本端IP地址,为空Server不显示
	REG_ClientPORT,  	     //15    数据采集器的本端端口
	REG_WIFIMAC,  	     		 //16    数据采集器连入互联网的网卡MAC地址  WiFi mac
	REG_ServerIP,  	     	 //17    数据采集器的远端（网络服务器）IP地址
	REG_ServerPORT,		 	 //18    数据采集器的远端（网络服务器）端口
	REG_ServerURL, 		 	 //19    数据采集器的远端（网络服务器）域名

	REG_GTSW, 				 //20    数据采集器的安规类型
	REG_SVersion,	    	 //21    数据采集器的固件版本
	REG_HVersion,            //22    数据采集器的硬件版本
	REG_ZigbeeID,            //23    Zigbee网络ID
	REG_ZigbeePV,            //24    Zigbee网络频道号
	REG_MASK,        		 //25    数据采集器的子网掩码
	REG_DG,          		 //26    数据采集器的默认网关
	REG_WIRELESSTYPE,        //27    无线模块类型设置,0：zigbee,1：wifi,3:3G,4:2G
	REG_NULL1,				 //28    不使用,光伏外围设备（外设）的使能设置。这几个外设在出厂状态是不开启的，默认值是0,0,0,# 0,0,0,# 0,0,0,# 0,0,0,#
	REG_NULL2,               //29    预留参数
	REG_GMT8,                //30    时区
	REG_TIME,  				 //31    数据采集器的系统时间
	REG_RESETFLAG,           //32    数据采集器重启标识,0：正常工作,1：重启数据采集器
	REG_CLEAR,               //33    清除ShinePano所有数据记录的标识,0：正常工作,1：清除ShinePano所有数据记录
	REG_CORRECTING,          //34    校正ShinePano触摸屏标识,0：正常工作
	REG_FACTORY,             //35    恢复数据采集器的出厂设置标识,0：正常工作
	REG_PollingFLAG,         //36    对光伏设备停止数据轮询的标识,0：进行光伏设备的轮询
	REG_03SUBSECTION,        //37    采集器对逆变器03号功能码的轮询区段定义.默认两个区段：0,44#45,89#
	REG_04SUBSECTION,        //38    04号功能码的轮询区段定义,同上
	REG_NULL3,               //39    预留参数
	REG_MONITORINGNUM,       //40    限制数据采集器对逆变器监控的数量。默认为0，有效值为[1,32]，其它值非法
	REG_COMRESET,			 //41    通讯保护时间。当数据采集器从逆变器侧获取不到数据，持续一段时间后，执行自重启操作。该参数用来设置和读取这段时间值。该功能用以缓解逆变器串口被打死的问题。
	REG_ProtocolsMode,       //42    数据传输模式。0：协议传输模式,1：透明传输模式
	REG_NULL4,			     //43    预留,Webbox使用
	REG_INUPGRADE,           //44	逆变器升级选择
	REG_NETTYPE,             //45	入网类型,1:WIFI.3:3G,0:自动4:,2G
	REG_NULL5,               //46
	REG_NULL6,               //47
	REG_NULL7,               //48
	REG_NULL8,               //49
	REG_NULL9,               //50
	REG_NULL10,              //51
	REG_NULL11,              //52
	REG_BLEMAC,    		     //53   蓝牙 mac
	REG_BLETOKEN,			 //54   蓝牙授权密钥
	REG_WIFISTATUS,			 //55   wifi连接状态
	REG_WIFISSID,			 //56   ShineWIFI连接的无线路由SSID
	REG_WIFIPASSWORD,		 //57	要连接的WIFI密码
	REG_WIFIPSK,			 //58	WIFI配置信息  默认加密方式 WIFI_AUTH_WPA2_PSK
	REG_3GSIGNAL,			 //59	2G,3G信号值
	REG_LINKSTATUS,			 //60	采集器与server 通信状态03/04/16
	REG_FirmwareVersion,	 //61	使用的三方模块的软件版本
	REG_SIMIMEI,			 //62	基带IMEI号码
	REG_SIMICCID,			 //63	SIM ICCID号码
	REG_SIMMMO,				 //64	SIM卡所属运营商
	REG_WIFIupdate,			 //65	wifi文件升级类型
	REG_ANTIREFLUX,			 //66	防逆流开关
	REG_ANTIREFLUXPOWER,	 //67	防逆流功率设置
	REG_RFCHANNEL,			 //68	RF通讯信道增量,默认为0
	REG_RFPAIR,				 //69	配对是否成功
	REG_WIFIMODE,			 //70	WIFI模式切换(AP/STA)
	REG_WIFIDHCP,			 //71	DHCP使能
	REG_MeboostMODE,		 //72	Meboost工作模式
	REG_MeboostFORCE,		 //73	Meboost强制烧水时间
	REG_MeboostHAND,		 //74	Meboost手动模式工作时长
	REG_WIFISCAN,			 //75	获取周围wifi名字   扫描wifi
	REG_SIGNAL,				 //76	最近200次信号的平均值
	REG_RECOVERIP,			 //77	是否使能连回服务器功能
	REG_ServerIP2,			 //78	预留服务器地址
	REG_ServerPORT2,		 //79	预留服务器端口
	REG_ServerURL2,			 //80	HTTP下载业务URL
	REG_DEBUGNUM1,			 //81	系统运行时间（单位：小时）
	REG_DEBUGNUM2,			 //82	系统上电次数
	REG_DEBUGNUM3,			 //83	系统软自重启次数
	REG_DEBUGNUM4,			 //84	成功连接服务器次数
	REG_DEBUGNUM5,			 //85	采集器升级成功次数
	REG_DEBUGNUM6,			 //86	采集器升级失败次数
	REG_DEBUGNUM7,			 //87	逆变器升级成功次数
	REG_DEBUGNUM8,			 //88	逆变器升级失败次数
	REG_DEBUGNUM9,			 //89
	REG_DEBUGNUM10,			 //90
	REG_DEBUGNUM11,			 //91
	REG_DEBUGNUM12,			 //92
	REG_DEBUGNUM13,			 //93
	REG_DEBUGNUM14,			 //94
	REG_DEBUGNUM15,			 //95
	REG_DEBUGNUM16,			 //96
	REG_DEBUGNUM17,			 //97
	REG_DEBUGNUM18,			 //98
	REG_DEBUGNUM19,			 //99
	REG_PROGRESS,			 //100

};

#define Parameter_len_MAX  102
#define SysTAB_Parameter_max  4096


//BLE 蓝牙关闭时间
#define DELAY_SCALE_MINUTE		       			60  						 //1分钟为60s
#define BLE_CLOSE_TIME_NOT_CONNECTED   			(15 * DELAY_SCALE_MINUTE)   //按键开启蓝牙 15 分钟后 关闭
#define BLE_CLOSE_TIME_CONNECTED_WHEN_RECV_DATA  (15 * DELAY_SCALE_MINUTE)    //正常收到数据后 15 分钟 关闭
#define BLE_CLOSE_TIME_DISCONNECT   			(10)  			 			//蓝牙连接断开 10s 后关闭


#define SMARTCONFIG_CLOSE_DELAY_TIME   			(30)  			 			//标准配网模式获取到wifi名称和密码后 的 自动关闭时间
#define HEARTBEAT_TIMEOUT						(5 * DELAY_SCALE_MINUTE)	//心跳包检测超时时间 5 分钟
#define INVERTER_RT_DATA_TIMEOUT				(30 * DELAY_SCALE_MINUTE)	//实时数据超时时间 30 分钟
#define CONNECT_SERVER_TIMEOUT				    (2 * 60  * DELAY_SCALE_MINUTE)	//实时数据超时时间 30 分钟


extern uint16_t Parameter_len[Parameter_len_MAX]; 	 				//二维长度映射表  数据地址
extern SYStem_state System_state;
extern uint8_t Collector_Config_LenList[Parameter_len_MAX]; 		//记录每个参数数据的长度
//extern char System_Config[REG_NUM_MAX][REG_LEN];					//采集器寄存器参数
extern char SysTAB_Parameter[4096];

extern uint8_t _GucServerParamSetFlag  ;        // 设置服务器参数：IP或端口标志位
extern char _GucServerIPBackupBuf[50] ; 		// 服务器IP地址备份区
extern char _GucServerPortBackupBuf[6]  ;		// 服务器端口备份区
extern const char BLEkeytoken[35] ;

extern char _ucServerIPBuf[50] ; 		// 远程设置服务器IP地址
extern char _ucServerPortBuf[6] ;		// 远程设置服务器端口
extern uint8_t uSetserverflag; 			// 判断是否使用备份的IP 连接

uint8_t IOT_ParameterTABInit(void);
uint8_t IOT_SystemParameterSet(uint16_t ParamValue ,char* data,uint8_t len);
uint8_t IOT_SystemDATA_init(void);

uint8_t IOT_SETConnect_uc(uint8_t Settype);


void IOT_FactoryReset(void);
void IOT_KEYlongReset(void);

void IOT_SystemFunc_Init(void);

void IOT_SetInvTime0x10(uint16_t TimeStartaddr,uint16_t TimeEndaddr ,uint8_t *pSetTimer);
uint8_t IOT_ServerConnect_Check(uint8_t type) ;
uint8_t IOT_ServerParamSet_Check(uint8_t mode);
uint16_t IOT_CollectorParam_Get(uint8_t paramnum,uint8_t *src);

void IOT_SysTEM_Task(void *arg);

void IOT_UPDATETime(void);
void ClearAllRecord(void);

void IOT_SYSRESTART(void);
uint8_t IOT_BLEkeytokencmp(void);

uint32_t GetSystemTick_ms(void);

//////////离线数据函数//////////////
void Write_RecordItem(uint8_t *ItemPoint);
uint16_t readRecord(uint8_t *tmp);
uint8_t Read_RecordItem(uint8_t *ItemPoint);
uint8_t Get_RecordStatus(void);
uint16_t Get_RecordNum(void);
void FlashData_Check(uint8_t *buf);
void IOT_Reset_Reason_Printf(void);

#endif /* COMPONENTS_INCLUDES_IOT_SYSTEM_H_ */
