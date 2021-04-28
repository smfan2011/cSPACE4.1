#include <comDef.h>
#include <eth_ctrl.h>

static ec_pdo_entry_info_t slave_pdo_entries[] = {
    {0x6040, 0x00, 16},         /* Control word */
    {0x607a, 0x00, 32},         /* Target position */
    //{0x6081, 0x00, 32},       /* Profile velocity */
    {0x60ff, 0x00, 32},         /* Target velocity */
    {0x60b1, 0x00, 32},         /* velocity  offset*/
    {0x60b2, 0x00, 16},         /* Torque offset*/
    {0x6071, 0x00, 16},         /* Target Torque*/
    {0x6060, 0x00, 8},          /* Operating mode*/

    {0x6041, 0x00, 16},         /* Status word */
    {0x6064, 0x00, 32},         /* Position actual value */
    {0x60f4, 0x00, 32},         /*Position loop error */
    {0x606c, 0x00, 32},         /* Velocity actual value */
    {0x6077, 0x00, 16},         /* Torque actual value */
    {0x6061, 0x00, 8},          /* Display Operating mode*/
};

static ec_pdo_info_t slave_pdos[] = {
    {0x1600, 7, slave_pdo_entries + 0}, /* Outputs */
    {0x1a00, 6, slave_pdo_entries + 7}, /* Inputs */
};

static ec_sync_info_t slave_syncs[]=
{
        {0, EC_DIR_OUTPUT, 0, NULL,EC_WD_DISABLE},
        {1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE},
        {2, EC_DIR_OUTPUT, 1, slave_pdos + 0, EC_WD_DISABLE},
        {3, EC_DIR_INPUT, 1, slave_pdos + 1, EC_WD_DISABLE},
        {0xff}
};

#if SDO_ACCESS
static ec_sdo_request_t *sdo[JOINTS_NUM];
static ec_sdo_request_t *request[JOINTS_NUM];
#endif

static unsigned int counter = 0;
static unsigned int sync_ref_counter = 0;

static ec_master_t *master = NULL;
static ec_master_state_t master_state = {};

static ec_domain_t *domain[JOINTS_NUM] = {NULL};
static ec_domain_state_t domain_state[JOINTS_NUM] = {};

static ec_slave_config_t *sc[JOINTS_NUM] = {NULL};
static ec_slave_config_state_t sc_state[JOINTS_NUM] = {};

static uint8_t *domain_pd[JOINTS_NUM] = {NULL};

static uint8_t g_model_type[JOINTS_NUM] = {0};
/****************************************************************************/
//控制模式
//uint8_t g_model_type[JOINTS_NUM] = {0};

/****************CSP参数配置**********************************************/
 //ec_pdo_entry_reg_t ** ethercat_pdo_config_init(uint8_t joint,ec_pdo_entry_reg_t **domain_regs,uint8_t n_entries)
void  ethercat_pdo_config_init(uint8_t joint,ec_pdo_entry_reg_t **domain_regs,uint8_t n_entries)
 {
	 int i;
	 
	 for(i=0;i<n_entries;i++){
#if MECH_GRIP
		if(joint < JOINTS_NUM-1){
			print_level(DEBUG, "ethercat_pdo_config_init[%d]: 0x%X %d\n",joint,slave_pdo_entries[i].index,n_entries);	
			domain_regs[joint][i].alias = TECH_ALIAS;
			domain_regs[joint][i].position = TECH_POSITION+joint;
			domain_regs[joint][i].vendor_id = TECH_VENDORID;
			domain_regs[joint][i].product_code = TECH_PRODUCTID;
			domain_regs[joint][i].index = slave_pdo_entries[i].index;
			domain_regs[joint][i].subindex = slave_pdo_entries[i].subindex;
			domain_regs[joint][i].offset = &TechservoDrive[joint].pdo_conf.slave_6040_00+i;
		}else{
			print_level(DEBUG, "ethercat_pdo_config_init[%d]: 0x%X %d\n",joint,dio_pdo_entries[i].index,n_entries);	
			domain_regs[joint][i].alias = IO_0_ALIAS;
			domain_regs[joint][i].position = IO_0_POSITION+joint;
			domain_regs[joint][i].vendor_id = IO_0_VENDORID;
			domain_regs[joint][i].product_code = IO_0_PRODUCTID;
			domain_regs[joint][i].index = dio_pdo_entries[i].index;
			domain_regs[joint][i].subindex = dio_pdo_entries[i].subindex;
			domain_regs[joint][i].offset = &TechservoDrive[joint].pdo_conf.off_bytes_led_val;
			domain_regs[joint][i].bit_position = &TechservoDrive[joint].pdo_conf.off_bits_led_val;	
		}
#else
		print_level(DEBUG, "ethercat_pdo_config_init[%d]: 0x%X %d\n",joint,slave_pdo_entries[i].index,n_entries);	
		domain_regs[joint][i].alias = TECH_ALIAS;
		domain_regs[joint][i].position = TECH_POSITION+joint;
		domain_regs[joint][i].vendor_id = TECH_VENDORID;
		domain_regs[joint][i].product_code = TECH_PRODUCTID;
		domain_regs[joint][i].index = slave_pdo_entries[i].index;
		domain_regs[joint][i].subindex = slave_pdo_entries[i].subindex;
		domain_regs[joint][i].offset = &TechservoDrive[joint].pdo_conf.slave_6040_00+i;
#endif
		
	 }

	 return;
	
 }

struct timespec timespec_add(struct timespec time1, struct timespec time2)
{
	struct timespec result;

	if ((time1.tv_nsec + time2.tv_nsec) >= NSEC_PER_SEC) {
		result.tv_sec = time1.tv_sec + time2.tv_sec + 1;
		result.tv_nsec = time1.tv_nsec + time2.tv_nsec - NSEC_PER_SEC;
	} else {
		result.tv_sec = time1.tv_sec + time2.tv_sec;
		result.tv_nsec = time1.tv_nsec + time2.tv_nsec;
	}

	return result;
}

/*****************************************************************************/

void check_domain_state(void)
{
    ec_domain_state_t ds;
	for(int i = 0; i < JOINTS_NUM; i++){
		memset(&ds, 0, sizeof(ec_domain_state_t));
		ecrt_domain_state(domain[i], &ds);
		#if 0
		if (ds.working_counter != domain_state[i].working_counter)
			print_level(DEBUG, "Domain[%d]: WC %u.\n", i, ds.working_counter);
		if (ds.wc_state != domain_state[i].wc_state)
			print_level(DEBUG, "Domain[%d]: State %u.\n", i, ds.wc_state);
		#endif	
		domain_state[i] = ds;
	}
}

/*****************************************************************************/

void check_master_state(void)
{
    ec_master_state_t ms;

    ecrt_master_state(master, &ms);
	#if 0
	if (ms.slaves_responding != master_state.slaves_responding)
        print_level(DEBUG, "%u slave(s).\n", ms.slaves_responding);
    if (ms.al_states != master_state.al_states)
        print_level(DEBUG, "AL states: 0x%02X.\n", ms.al_states);
    if (ms.link_up != master_state.link_up)
        print_level(DEBUG, "Link is %s.\n", ms.link_up ? "up" : "down");
	#endif
    master_state = ms;
}

/*****************************************************************************/
void check_slave_config_states(void)
{
    ec_slave_config_state_t s;
	
	for(int i = 0; i < JOINTS_NUM; i++) {
		memset(&s, 0, sizeof(ec_slave_config_state_t));
		ecrt_slave_config_state(sc[i],&s);
		#if 0
		if (s.al_state != sc_state[i].al_state)
			print_level(DEBUG, "sc_state[%d]: State 0x%02X.\n", i, s.al_state);
		if (s.online != sc_state[i].online)
			print_level(DEBUG,"sc_state[%d]: %s.\n", i, s.online ? "online" : "offline");
		if (s.operational != sc_state[i].operational)
			print_level(DEBUG, "sc_state[%d]: %s operational.\n", i, s.operational ? "" : "Not ");
		#endif
		sc_state[i] = s;
	}
}

/*****************************************************************************/
#if SDO_ACCESS

uint32_t read_sdo(ec_sdo_request_t *req)
{
	uint32_t sdo_ret;
    switch (ecrt_sdo_request_state(req)) {
        case EC_REQUEST_UNUSED: // request was not used yet
            ecrt_sdo_request_read(req); // trigger first read
            break;
        case EC_REQUEST_BUSY:
            //print_level(ERROR,  "SDO still busy...\n");
            break;
        case EC_REQUEST_SUCCESS:
			sdo_ret = EC_READ_U32(ecrt_sdo_request_data(req));
            //print_level(DEBUG, "SDO value read: 0x%X\n", sdo_ret);
            ecrt_sdo_request_read(req); // trigger next read
            break;
        case EC_REQUEST_ERROR:
            print_level(ERROR,  "Failed to read SDO!\n");
            ecrt_sdo_request_read(req); // retry reading
            break;
    }
	return sdo_ret;
}

void write_sdo(ec_sdo_request_t *req, unsigned data)
{
	EC_WRITE_U32(ecrt_sdo_request_data(req), data&0xffffffff);
	switch (ecrt_sdo_request_state(req)) {
		case EC_REQUEST_UNUSED: // request was not used yet
			ecrt_sdo_request_write(req); // trigger first read
			break;
		case EC_REQUEST_BUSY:
			print_level(ERROR,  "SDO write still busy...\n");
			break;
		case EC_REQUEST_SUCCESS:
			print_level(DEBUG, "SDO value written: 0x%X\n", data);
			ecrt_sdo_request_write(req); // trigger next read ???
			break;
		case EC_REQUEST_ERROR:
			print_level(ERROR,  "Failed to write SDO!\n");
			ecrt_sdo_request_write(req); // retry writing
			break;
	}
}
#endif

/****************************************************************************/
static uint32_t val = 0;
#if 0
static int8_t angle_flag[JOINTS_NUM] = {0};

static uint8_t jo_flag[JOINTS_NUM] = {0};
static uint8_t bake_flag[JOINTS_NUM] = {0};
#endif
static uint8_t bake_flag[JOINTS_NUM] = {0};
static int8_t angle_flag[JOINTS_NUM] = {0};
void EC_CyclicTask(struct timespec *wakeupTime, struct timespec *time, Motor_ctrl *m_ctrl)
{
	int i=0;
	uint8_t model_type[JOINTS_NUM];
	unsigned long long int u32_pos = 0;

#if (ALG_CTRl == 0)
	
	*wakeupTime = timespec_add(*wakeupTime, cycletime);
  	 clock_nanosleep(CLOCK_TO_USE, TIMER_ABSTIME, wakeupTime, NULL);
#endif
	ecrt_master_application_time(master, TIMESPEC2NS(*wakeupTime));

	// receive process data
	ecrt_master_receive(master);
		
	for(i = 0; i < JOINTS_NUM; i++){
		ecrt_domain_process(domain[i]);
	}
	
	// check process data state (optional)
	check_domain_state();

	if (counter) {
		counter--;
	} 
	else { // do this at 1 Hz
		//counter = FREQUENCY;
		counter = 1000;
		// check for master state (optional)
		check_master_state();
		check_slave_config_states();
		
		val = !val;
	}

/*********************************************************/
#if MECH_GRIP
	for(i = 0; i < JOINTS_NUM-1; i++){
#else
	for(i = 0; i < JOINTS_NUM; i++){
#endif
		//状态机操作
		switch (g_MS_WorkStatus){
		case MS_SAFE_MODE:
			//检查主站是否处于 OP 模式, 若不是，则调整为 OP 模式
			check_master_state();
			check_slave_config_states();
			if((master_state.al_states & MS_STATUS_OP))
			{
				if(sc_state[i].al_state != MS_STATUS_OP)
				{
					break ;
				}
				g_MS_WorkStatus = MS_OP_MODE;
				EC_WRITE_U16(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6040_00, 0x80);    //错误复位 
			}
		break;
		case MS_OP_MODE:
			//读0x603F错误寄存器，有错误立即停机
			/*for(int j = 0; j < 16; j++){
				if(read_sdo(request[i]) == err_code[j].value){
					EC_WRITE_U16(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6040_00, 0x00);
					shm_send.JointInfo[i].error = err_code[j].value;							//错误码发送to算法
					goto _ERR_CODE;
				}
			}*/
			//切换模式
			#if ALG_CTRL
			model_type[i] = shm_recv.JointInfo[i].model_type/*MODEL_CSP*/;
			#else
			model_type[i] = /*shm_recv.JointInfo[i].model_type*/MODEL_CSP;
			#endif
			if((model_type[i] != 0) && (g_model_type[i] != model_type[i])){
				EC_WRITE_S8(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6060_00, model_type[i]); 	//切换模式
			}
			g_model_type[i]= EC_READ_S8(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6061_00);
			
			#if 1
			if(g_model_type[i] == MODEL_PP || g_model_type[i] == MODEL_CSP || g_model_type[i] == MODEL_PV || g_model_type[i] == MODEL_CSV || g_model_type[i] == MODEL_PT|| g_model_type[i] == MODEL_CST){
				Pos_act_value[i]=EC_READ_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6064_00);  //读取实际位置		
				shm_send.JointInfo[i].pdo_val.actual_pos = Pos_act_value[i];							//当前位置发送to算法	
#if 0
				if(i == 0){
					u32_pos++;
					if(u32_pos == 500000000){
						printf("shm_send.JointInfo[%d].pdo_val.actual_pos:%d\n", i, shm_send.JointInfo[i].pdo_val.actual_pos);
						u32_pos = 0;
					}
				}
#endif
				Vel_act_value[i]=EC_READ_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_606c_00);  //读取实际速度		
				shm_send.JointInfo[i].pdo_val.actual_vel = Vel_act_value[i];							//当前速度发送to算法	
				Torq_act_value[i]=EC_READ_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6077_00);  //读取实际力矩		
				shm_send.JointInfo[i].pdo_val.actual_torq = Torq_act_value[i];							 //当前力矩发送to算法		
			}else{
				goto _ERR_CODE;
			}
			#else 
			if(g_model_type[i] == MODEL_PP || g_model_type[i] == MODEL_CSP){
				Pos_act_value[i]=EC_READ_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6064_00);  //读取实际位置		
				shm_send.JointInfo[i].pdo_val.actual_pos = Pos_act_value[i];							//当前位置发送to算法	
			}
			else if (g_model_type[i] == MODEL_PV || g_model_type[i] == MODEL_CSV){
				Vel_act_value[i]=EC_READ_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_606c_00);  //读取实际速度		
				shm_send.JointInfo[i].pdo_val.actual_vel = Vel_act_value[i];							//当前速度发送to算法	
			}
			else if(g_model_type[i] == MODEL_PT|| g_model_type[i] == MODEL_CST){
				Torq_act_value[i]=EC_READ_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6077_00);  //读取实际力矩		
				shm_send.JointInfo[i].pdo_val.actual_torq = Torq_act_value[i];							 //当前力矩发送to算法		
			}
			else{
				goto _ERR_CODE;																			//退出
			}
			#endif
			status_word[i]=EC_READ_U16(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6041_00);		//读取状态码
			shm_send.JointInfo[i].pdo_val.status_word = status_word[i];									//状态码发送to算法
		
			if( (status_word[i]&0x004f) == 0x0040)
			{
				#if ALG_CTRL
				EC_WRITE_U16(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6040_00, /*0x0006*/shm_recv.JointInfo[i].pdo_val.ctrl_word);
				#else
				EC_WRITE_U16(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6040_00, 0x0006/*shm_recv.JointInfo[i].pdo_val.ctrl_word*/);
				#endif
				//printf("step 1 done");
			}
			else if( (status_word[i]&0x006f) == 0x0021)
			{
				#if ALG_CTRL
				EC_WRITE_U16(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6040_00, /*0x0007*/shm_recv.JointInfo[i].pdo_val.ctrl_word);
				#else
				EC_WRITE_U16(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6040_00, 0x0007/*shm_recv.JointInfo[i].pdo_val.ctrl_word*/);
				#endif
				//printf("step2 done! \n");
			}
			else if( (status_word[i]&0x006f) == 0x023 && !bake_flag[i])
			{
				EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_607a_00, Pos_act_value[i]);
				#if ALG_CTRL
				EC_WRITE_U16(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6040_00, /*0x000f*/shm_recv.JointInfo[i].pdo_val.ctrl_word);
				#else
				EC_WRITE_U16(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6040_00, 0x000f/*shm_recv.JointInfo[i].pdo_val.ctrl_word*/);
				#endif
				bake_flag[i] = 1;	//抱闸已开启标志
				//printf("step 3 done\n");			
			}
			else if( (status_word[i]&0x006f) == 0x027/* && bake_flag[i]*/)
			{
				switch (g_model_type[i]){
					case MODEL_PP:
						Target_pos_value[i] = shm_recv.JointInfo[i].pdo_val.target_pos/*m_ctrl->angle[i]*JOINTS_PULSE*/;				//获取目标位置
						EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_607a_00, Target_pos_value[i]); // PP mode
					break;
					case MODEL_CSP:
						#if ALG_CTRL
						Target_pos_value[i] = shm_recv.JointInfo[i].pdo_val.target_pos/*m_ctrl->angle[i]*JOINTS_PULSE*/;				//获取目标位置
						EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_607a_00, Target_pos_value[i]);
						#else //算法端进行规划
						Target_pos_value[i] = m_ctrl->angle[i]*JOINTS_PULSE;						//获取目标位置
						angle_flag[i] = (Target_pos_value[i] >= Pos_act_value[i]) ? 1:-1;			//判断伺服转向
						if(abs(Pos_act_value[i] - Target_pos_value[i]) < 100){
							EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_607a_00, Target_pos_value[i] + 0);
						}else{
							EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_607a_00, Pos_act_value[i] + OFFSET_VALUE*angle_flag[i]); 	// CSP mode
						}
						#endif
					break;
					case MODEL_PV:
						Target_vel_value[i] = shm_recv.JointInfo[i].pdo_val.target_vel;				//获取目标速度
						EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_60ff_00, Target_vel_value[i]); // PV mode						
					break;
					case MODEL_CSV:
						#if ALG_CTRL
						Target_vel_value[i] = shm_recv.JointInfo[i].pdo_val.target_vel;				//获取目标速度
						EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_60ff_00, Target_vel_value[i]);
						#else
						Target_vel_value[i] = shm_recv.JointInfo[i].pdo_val.target_vel;				//获取目标速度
						angle_flag[i] = (Target_vel_value[i] >= Vel_act_value[i]) ? 1:-1;			//判断伺服转向
						if(abs(Vel_act_value[i] - Target_vel_value[i]) < 100){
							EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_60ff_00, Target_vel_value[i] + 0);
						}else{
							EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_60ff_00, Vel_act_value[i] + OFFSET_VALUE*angle_flag[i]); 	// CSV mode
						}
						#endif
					break;
					case MODEL_PT:
						Target_torq_value[i] = shm_recv.JointInfo[i].pdo_val.target_torq;			//获取目标力矩
						EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6071_00, Target_torq_value[i]); // PT mode						
					break;
					case MODEL_CST:
						#if ALG_CTRL
						Target_torq_value[i] = shm_recv.JointInfo[i].pdo_val.target_torq;			//获取目标力矩
						EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6071_00, Target_torq_value[i]);
						#else
						Target_torq_value[i] = shm_recv.JointInfo[i].pdo_val.target_torq;			//获取目标力矩
						angle_flag[i] = (Target_torq_value[i] >= Torq_act_value[i]) ? 1:-1;			//判断伺服转向
						if(abs(Torq_act_value[i] - Target_torq_value[i]) < 100){
							EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6071_00, Target_torq_value[i] + 0);
						}else{
							EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6071_00, Torq_act_value[i] + OFFSET_VALUE*angle_flag[i]); 	// CST mode
						}
						#endif						
					break;
					default:
						EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_607a_00, Pos_act_value[i]); 
						EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_60ff_00, Vel_act_value[i]); 
						EC_WRITE_S32(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6071_00, Torq_act_value[i]); 
					break;
				}
				//启动运动控制
				#if ALG_CTRL
				EC_WRITE_U16(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6040_00, shm_recv.JointInfo[i].pdo_val.ctrl_word/*0x001f*/);			/*0x001f*/
				#else
				EC_WRITE_U16(domain_pd[i] + TechservoDrive[i].pdo_conf.slave_6040_00, 0x001f);			/*0x001f*/
				#endif
			}
		break;
		default:
			goto _ERR_CODE;
		}	
	}

#if MECH_GRIP
		EC_WRITE_U8(domain_pd[i] + TechservoDrive[i].pdo_conf.off_bytes_led_val, val ? 0x01:0x02); // CSV mode
#endif
	// write application time to master
	//clock_gettime(CLOCK_TO_USE, time);
	//ecrt_master_application_time(master, TIMESPEC2NS(*time));
_ERR_CODE:
	if (sync_ref_counter) {
		sync_ref_counter--;
	} else {
		sync_ref_counter = 0; // sync every cycle
		ecrt_master_sync_reference_clock(master);
	}
	ecrt_master_sync_slave_clocks(master);

	// send process data
	for(int i = 0; i < JOINTS_NUM; i++)
		ecrt_domain_queue(domain[i]);

	ecrt_master_send(master);
}


//配置PDO
int EC_ConfigPDO()
{
	//ec_slave_config_t *sc[JOINTS_NUM];
	int i=0;
	memset(g_model_type, MODEL_PP, sizeof(g_model_type));
	uint32_t curveSpeed = 0x8000;		//轨迹速度
	int16_t curveType = 0x03;			//S曲线模式
	
	uint8_t n_entries = sizeof(slave_pdo_entries)/sizeof(slave_pdo_entries[0]);
#if MECH_GRIP
	uint8_t n_dio_entries = sizeof(dio_pdo_entries)/sizeof(dio_pdo_entries[0]);
#endif
	ec_pdo_entry_reg_t **domain_regs;
	domain_regs = (ec_pdo_entry_reg_t**)malloc(sizeof(ec_pdo_entry_reg_t*) * JOINTS_NUM); 
	/*
	for(int i = 0; i < JOINTS_NUM; i++) {
		domain_regs[i] = (ec_pdo_entry_reg_t*)malloc(sizeof(ec_pdo_entry_reg_t) * (n_entries+1));
		ethercat_pdo_config_init(i,domain_regs,n_entries);	
	}
	*/
	print_level(DEBUG, "xenomai Configuring PDOs...\n");
#if MECH_GRIP
	for(i = 0; i < JOINTS_NUM-1; i++) {
#else
	for(i = 0; i < JOINTS_NUM; i++) {
#endif
		domain[i] = ecrt_master_create_domain(master);
		if (!domain[i])
			return -1;
			
	    // Create configuration for bus coupler
		sc[i] = ecrt_master_slave_config(master, TECH_ALIAS, TECH_POSITION+i, Techservo);
		if (!sc[i])
			return -1;		

		print_level(INFO, "Configuring PDOs...\n");
		domain_regs[i] = (ec_pdo_entry_reg_t*)malloc(sizeof(ec_pdo_entry_reg_t) * (n_entries+1));
		ethercat_pdo_config_init(i,domain_regs,n_entries);	
		
		if (ecrt_slave_config_pdos(sc[i], EC_END, slave_syncs)) 
		{
			print_level(ERROR,  "Failed to configure 1st PDOs.\n");
			return -1;
		}	

		for(int j=0;j<n_entries;j++){
			print_level(DEBUG, "ethercat_pdo_config_init after[%d]: 0x%X\n",j,domain_regs[i][j].index);	
		}

	    if (ecrt_domain_reg_pdo_entry_list(domain[i], domain_regs[i])) 
		{
			print_level(ERROR,  "first motor PDO entry registration failed!\n");
			return -1;
		}	
		#if 1
		
		ecrt_slave_config_sdo8(sc[i],0x6060,0,g_model_type[i]);
		ecrt_slave_config_sdo16(sc[i],0x6086,0,curveType);
		ecrt_slave_config_sdo32(sc[i],0x6081,0,curveSpeed);
		/*
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd,"ethercat download -t uint8 -p %d 0x6060 0 %d",i,g_model_type[i]);
		system(cmd);
		print_level(INFO, "%s\n", cmd);
		
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd,"ethercat download -t uint32 -p %d 0x6081 0 %d",i,curveSpeed);
		system(cmd);
		print_level(INFO, "%s\n", cmd);
		
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd,"ethercat download -t uint16 -p %d 0x6086 0 %d",i,curveType);
		system(cmd);
		print_level(INFO, "%s\n", cmd);
*/
		
		#else
		//操作模式=默认PP
		if (ecrt_master_sdo_download(master, i, 0x6060, 0x00, ((uint8_t *)(g_model_type+i)), 1, &abort_code))
		{
			 print_level(ERROR,  "EtherCAT Failed to initialize slave techservo EtherCAT at alias 0. Error: %d\n", abort_code);
			 return -1;
		}
		
		//限制曲线速度=0x20000
		if (ecrt_master_sdo_download(master, i, 0x6081, 0x00, (uint32_t)(&curveSpeed), 4, &abort_code))
		{
			 print_level(ERROR,  "EtherCAT [0x6081] Failed to initialize slave techservo EtherCAT at alias 0. Error: %d\n", abort_code);
			 return -1;
		}
		
		if (ecrt_master_sdo_download(master, i, 0x6086, 0x00, (int8_t *)(&curveType), 2, &abort_code))
		{
			 print_level(ERROR,  "EtherCAT [0x6086] Failed to initialize slave techservo EtherCAT at alias 0. Error: %d\n", abort_code);
			 return -1;
		}
		
		if (!(request[i] = ecrt_slave_config_create_sdo_request(sc[i], 0x603F, 0, 2))) {
			print_level(ERROR,  "Failed to create SDO request 0x603F.\n");
			return -1;
		}
		ecrt_sdo_request_timeout(request[i], 500); // ms

		#endif
		#if 0
		if (!(sdo[i] = ecrt_slave_config_create_sdo_request(sc[i], 0x6041, 0, 2))) {
			print_level(ERROR,  "Failed to create SDO request 0x6041.\n");
			return -1;
		}
		ecrt_sdo_request_timeout(sdo[i], 500); // ms

		if (!(request[i] = ecrt_slave_config_create_sdo_request(sc[i], 0x1002, 0, 4))) {
			print_level(ERROR,  "Failed to create SDO request 0x1002.\n");
			return -1;
		}
		ecrt_sdo_request_timeout(request[i], 1000); // ms
		#endif
		// configure SYNC signals for this slave
		ecrt_slave_config_dc(sc[i], 0x0330, PERIOD_NS, 44000, 0, 0);
		//用完就释放
		free(domain_regs[i]);
	}
	
#if MECH_GRIP
		domain[i] = ecrt_master_create_domain(master);
		if (!domain[i])
			return -1;
			
	    // Create configuration for bus coupler
		sc[i] = ecrt_master_slave_config(master, IO_0_ALIAS, IO_0_POSITION+i, Techinfineon);
		if (!sc[0])
			return -1;	

		print_level(INFO, "Configuring PDOs...\n");
		domain_regs[i] = (ec_pdo_entry_reg_t*)malloc(sizeof(ec_pdo_entry_reg_t) * (n_dio_entries+1));
		ethercat_pdo_config_init(i,domain_regs,n_dio_entries);	
		
		if (ecrt_slave_config_pdos(sc[i], EC_END, dio_syncs)) 
		{
			print_level(ERROR,  "Failed to configure 1st PDOs.\n");
			return -1;
		}	

		for(int j=0;j<n_dio_entries;j++){
			print_level(DEBUG, "ethercat_pdo_config_init after[%d]: 0x%X\n",j,domain_regs[i][j].index);	
		}

	    if (ecrt_domain_reg_pdo_entry_list(domain[i], domain_regs[i])) 
		{
			print_level(ERROR,  "first motor PDO entry registration failed!\n");
			return -1;
		}
		ecrt_slave_config_dc(sc[i], 0x0330, 2000000, 4400000, 0, 0);
		//用完就释放
		free(domain_regs[i]);		

#endif

    free(domain_regs);
	
    print_level(DEBUG,  "Creating SDO requests...\n");

    return 0;
}


//释放主站
void EC_ReleaseMaster()
{
    if(master)
    {
        print_level(DEBUG, "xenomai End of Program, release master\n");
        ecrt_release_master(master);
        master = NULL;
    }
}

//主站激活
int EC_ActivateMaster()
{
    print_level(DEBUG, "xenomai Requesting master...\n");
    if(master)
        return 0;
    master = ecrt_request_master(0);
    if (!master) {
        return -1;
    }

    EC_ConfigPDO();

    /********************/
    print_level(INFO, "Activating master...\n");
    if (ecrt_master_activate(master)){
        return -1;
    }
    for(int i = 0; i < JOINTS_NUM; i++){
	if (!(domain_pd[i] = ecrt_domain_data(domain[i]))) {
			return -1;
	}
   }

   pid_t pid = getpid();
    if (setpriority(PRIO_PROCESS, pid, -19)) {
        print_level(ERROR,  "Warning: Failed to set priority: %s\n",strerror(errno));
	}
	
    print_level(INFO, "Activating master...success\n");
    return 0;
}

