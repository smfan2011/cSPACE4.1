/*
 * 	File: eth_ctrl.h
 *
 *  Created on: 2021年03月31日
 *  Author: becking
 *  Copyright (C) 2021 si Valley <www.si-valley.com>
 */
 
#ifndef ETH_CTRL_H
#define	ETH_CTRL_H

#include "comDef.h"
#include "ecrt.h"

/****************************************************************************/
// Application parameters
//#define FREQUENCY 500
#define CLOCK_TO_USE CLOCK_REALTIME
#define MEASURE_TIMING

#define CONFIGURE_PDOS 1

// Optional features
#define PDO_SETTING1	1
//Master Status
#define	MS_STATUS_OP	0x08

#define SDO_ACCESS		0

/****************************************************************************/

#define NSEC_PER_SEC (1000000000L)
#if (ALG_CTRL == 0)
//#define PERIOD_NS (NSEC_PER_SEC / FREQUENCY)
#endif

#define DIFF_NS(A, B) (((B).tv_sec - (A).tv_sec) * NSEC_PER_SEC + \
	(B).tv_nsec - (A).tv_nsec)

#define TIMESPEC2NS(T) ((uint64_t) (T).tv_sec * NSEC_PER_SEC + (T).tv_nsec)

/****************************************************************************/
//从设备别名,位置信息，根据手册定义
#define BusCouplerPos    0, 0
#define Techservo			0x000000ab, 0x00001030
#define TECH_VENDORID 		0x000000ab
#define TECH_PRODUCTID 		0x00001030
#define TECH_ALIAS          0x00
#define TECH_POSITION       0x00

#define BusCouplerPos0  0, 0
#define BusCouplerPos1  0, 1
#define BusCouplerPos2  0, 2
#define BusCouplerPos3  0, 3
#define BusCouplerPos4  0, 4
#define BusCouplerPos5  0, 5

//夹爪从设备别名,位置信息，根据手册定义
#define Techinfineon		0x0000034E, 0x00008000
#define IO_0_VENDORID  		0x0000034E
#define IO_0_PRODUCTID 		0x00008000
#define IO_0_ALIAS          0x00
#define IO_0_POSITION       0x00

typedef struct __PDO_CONF {
	unsigned int slave_6040_00;  // Contorl word
	unsigned int slave_607a_00;  // Profile target position
	unsigned int slave_60ff_00;  // Target velocity
	unsigned int slave_60b1_00;  // Velocity offset
	unsigned int slave_60b2_00;  // Torque offset
	unsigned int slave_6071_00;  // Target torque
	unsigned int slave_6060_00;  // Operating mode
	unsigned int slave_6041_00;  // status word
	unsigned int slave_6064_00;  // Actual motor position
	unsigned int slave_60f4_00;  // Position loop error	
	unsigned int slave_606c_00;  // Actual motor velocity
	unsigned int slave_6077_00;  // Torque actual value
	unsigned int slave_6061_00;  // Display operation mode
#if MECH_GRIP
	unsigned int off_bytes_led_val;
	unsigned int off_bits_led_val;
#endif
}PDO_CONF_T;

struct ServoDrive {
	PDO_CONF_T pdo_conf;		//PDO配置参数
	uint8_t error;			//错误码
	uint8_t status;			//状态
};

struct ServoDrive TechservoDrive[JOINTS_NUM];			//6个关节

/*****************************************************************************/
//配置pdo

#if 0

static ec_pdo_entry_info_t slave_pdo_entries[] = {
    {0x6040, 0x00, 16}, 	/* Control word */
    {0x607a, 0x00, 32}, 	/* Target position */
    //{0x6081, 0x00, 32}, 	/* Profile velocity */
    {0x60ff, 0x00, 32}, 	/* Target velocity */
    {0x60b1, 0x00, 32}, 	/* velocity  offset*/
    {0x60b2, 0x00, 16}, 	/* Torque offset*/
    {0x6071, 0x00, 16}, 	/* Target Torque*/
    {0x6060, 0x00, 8}, 		/* Operating mode*/
	
    {0x6041, 0x00, 16}, 	/* Status word */
    {0x6064, 0x00, 32}, 	/* Position actual value */
    {0x60f4, 0x00, 32}, 	/*Position loop error */
    {0x606c, 0x00, 32}, 	/* Velocity actual value */
    {0x6077, 0x00, 16}, 	/* Torque actual value */
    {0x6061, 0x00, 8}, 		/* Display Operating mode*/
};


/********************* CSP/CSV/CST**********/

static ec_pdo_info_t slave_pdos[] = {
    {0x1600, 7, slave_pdo_entries + 0}, /* Outputs */
    {0x1a00, 6, slave_pdo_entries + 7}, /* Inputs */
};
#endif

#if 0
static ec_sync_info_t slave_syncs[]= 
{
	{0, EC_DIR_OUTPUT, 0, NULL,EC_WD_DISABLE},
	{1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE},
	{2, EC_DIR_OUTPUT, 1, slave_pdos + 0, EC_WD_DISABLE},
	{3, EC_DIR_INPUT, 1, slave_pdos + 1, EC_WD_DISABLE},
	{0xff}
};
#endif

//机械夹爪
#if MECH_GRIP
static ec_pdo_entry_info_t dio_pdo_entries[] = {
	{0x7000, 0x01, 1}, /* LED1 */
    {0x7000, 0x02, 1}, /* LED2 */
    {0x7000, 0x03, 1}, /* LED3 */
    {0x7000, 0x04, 1}, /* LED4 */
    {0x7000, 0x05, 1}, /* LED5 */
    {0x7000, 0x06, 1}, /* LED6 */
    {0x7000, 0x07, 1}, /* LED7 */
    {0x7000, 0x08, 1}, /* LED8 */
};
/********************* CSP/CSV/CST**********/
static ec_pdo_info_t dio_pdos[] = {
	{0x1600, 8, dio_pdo_entries + 0}, /* LED process data mapping */
};

static ec_sync_info_t dio_syncs[] = 
{
	{0, EC_DIR_OUTPUT, 0, dio_pdos + 0, EC_WD_ENABLE},
	{0xff}
};
#endif

/**************************************************************************/
// EtherCAT

#if 0
static ec_master_t *master = NULL;
static ec_master_state_t master_state = {};

static ec_domain_t *domain[JOINTS_NUM] = {NULL};
static ec_domain_state_t domain_state[JOINTS_NUM] = {};

static ec_slave_config_t *sc[JOINTS_NUM] = {NULL};
static ec_slave_config_state_t sc_state[JOINTS_NUM] = {};

// process data
static uint8_t *domain_pd[JOINTS_NUM] = {NULL};

/****************************************************************************/
#if SDO_ACCESS
static ec_sdo_request_t *sdo[JOINTS_NUM];
static ec_sdo_request_t *request[JOINTS_NUM];
#endif

static unsigned int counter = 0;
static unsigned int sync_ref_counter = 0;
#endif
static const struct timespec cycletime = {0, PERIOD_NS};

/****************************************************************************/
//控制模式
#if 0
static uint8_t g_model_type[JOINTS_NUM] = {0};		
#endif

unsigned int abort_code;							//错误码
unsigned int status_word[JOINTS_NUM];				//伺服状态字

int32_t Pos_act_value[JOINTS_NUM];					//实际位置值
int32_t Target_pos_value[JOINTS_NUM];				//目标位置值

int32_t Vel_act_value[JOINTS_NUM];					//实际速度值
int32_t Target_vel_value[JOINTS_NUM];				//目标速度值

int32_t Torq_act_value[JOINTS_NUM];					//实际力矩值
int32_t Target_torq_value[JOINTS_NUM];				//目标力矩值

/**************************************************************************/
//错误码
#if 0
static struct ERR_CODE_T {
	char name[64];
	uint32_t value;
}err_code[16]= {
	{"短路",0x2320},
	{"驱动器过温",0x4210},
	{"驱动器过压",0x3110},
	{"驱动器欠压",0x3120},
	{"电机过温",0x4300},
	{"反馈错误",0x2280},
	{"相位错误",0x7122},
	{"电流限",0x2310},
	{"电压限",0x3310},
	{"正限位接通",0x7380},
	{"负限位接通",0x7381},
	{"跟随超差",0x7390},
	{"位置计数器到达",0x73A0},
	{"适用于故障无其他紧急情况",0x5080},
	{"节点错误",0x8130},
	{"命令错误",0x61FF}
};

#endif

typedef struct {
        Model_type model_type;  //控制模式
        Pdo_info pdo_val;               //PDO配置参数
        uint8_t error;                  //错误码
        uint8_t status;                 //状态
}Msg_info;

typedef struct {
    long type;
        Msg_info JointInfo[JOINTS_NUM];                 //6个关节
}Msg;

//MSG
Msg msg_send, msg_recv;

//master status
typedef enum  master_status
{
    MS_POWER_ON,
    MS_SAFE_MODE,
    MS_OP_MODE,
    MS_LINK_DOWN,
    MS_IDLE_STATUS       //系统空闲
}enum_Master_tatus;

enum_Master_tatus	g_MS_WorkStatus;

/***************************共享内存*********************************/
//激活主站
int EC_ActivateMaster();
//释放主站
void EC_ReleaseMaster();
//循环任务
void EC_CyclicTask(struct timespec *wakeupTime, struct timespec *time, Motor_ctrl *m_ctrl);

struct timespec timespec_add(struct timespec time1, struct timespec time2);
#endif
