#ifndef __EHRPWM_H__
#define __EHRPWM_H__

#include <linux/pwm/pwm.h>

/* TODO: should we make these defines into enums? */

/* TBLCTL (Time-Based Control) */

/* TBCNT Mode bits */
#define TB_COUNT_UP      0x0
#define TB_COUNT_DOWN    0x1
#define TB_COUNT_UP_DOWN 0x2
#define TB_FREEZE        0x3
/* PHSEN bit */
#define TB_DISABLE       0x0
#define TB_ENABLE        0x1
/* PRDLD bit */
#define TB_SHADOW        0x0
#define TB_IMMEDIATE     0x1
/* SYNCOSEL bits */
#define TB_SYNC_IN       0x0
#define TB_CTR_ZERO      0x1
#define TB_CTR_CMPB      0x2
#define TB_SYNC_DISABLE  0x3
/* HSPCLKDIV bits */
#define TB_HS_DIV1       0x0
#define TB_HS_DIV2       0x1
#define TB_HS_DIV4       0x2
#define TB_HS_DIV6       0x3
#define TB_HS_DIV8       0x4
#define TB_HS_DIV10      0x5
#define TB_HS_DIV12      0x6
#define TB_HS_DIV14      0x7
/* CLKDIV bits */
#define TB_DIV1          0x0
#define TB_DIV2          0x1
#define TB_DIV4          0x2
#define TB_DIV8          0x3
#define TB_DIV16         0x4
#define TB_DIV32         0x5
#define TB_DIV64         0x6
#define TB_DIV128        0x7
/* PHSRDIR bit */
#define TB_DOWN          0x0
#define TB_UP            0x1

/* CMPCTL (Compare Control) */

/* LOADAMODE and LOADBMODE bits */
#define CC_CTR_ZERO      0x0
#define CC_CTR_PRD       0x1
#define CC_CTR_ZERO_PRD  0x2
#define CC_LD_DISABLE    0x3
/* SHDWAMODE and SHDWBMODE bits */
#define CC_SHADOW        0x0
#define CC_IMMEDIATE     0x1

/* AQCCTLA and AQCTLB (Action-qualifier Control) */

/* channel */
#define AQ_CHA           0x0
#define AQ_CHB           0x1
/* ZRO, PRD, CAU, CAD, CBU and CBD bits */
#define AQ_NO_ACTION     0x0
#define AQ_CLEAR         0x1
#define AQ_SET           0x2
#define AQ_TOGGLE        0x3

/* DBCTL (Dead-Band Control) */

/* MODE bits */
#define DB_DISABLE       0x0
#define DBA_ENABLE       0x1
#define DBB_ENABLE       0x2
#define DB_FULL_ENABLE   0x3
/* POLESL bits */
#define DB_ACTIVE_HIGH   0x0
#define DB_ACTIVE_LOWC   0x1
#define DB_ACTIVE_HIGHC  0x2
#define DB_ACTIVE_LOW    0x3

/* PCCTL (Pulse-chopper control) */

/* CHPEN bit */
#define PC_DISABLE       0x0
#define PC_ENABLE        0x1
/* CHPFREQ bits */
#define PC_DIV1          0x0
#define PC_DIV2          0x1
#define PC_DIV3          0x2
#define PC_DIV4          0x3
#define PC_DIV5          0x4
#define PC_DIV6          0x5
#define PC_DIV7          0x6
#define PC_DIV8          0x7
/* CHPDUTY bits */
#define PC_1_8TH         0x0
#define PC_2_8TH         0x1
#define PC_3_8TH         0x2
#define PC_4_8TH         0x3
#define PC_5_8TH         0x4
#define PC_6_8TH         0x5
#define PC_7_8TH         0x6

/* TZSEL (Trip-zone Select) */

/* CBCn and OSHTn bits */
#define TZ_ENABLE        0x0
#define TZ_DISABLE       0x1

/* TZCTL (Trip-zone Control) */

/* TZA and TZB bits */
#define TZ_HIGHZ         0x0
#define TZ_FORCE_HIGH    0x1
#define TZ_FORCE_LOW     0x2
#define TZ_TZ_DISABLE    0x3

/* ETSEL (Event-trigger Select) */

/* INTEN bit */
#define ET_DISABLE       0x0
#define ET_ENABLE        0x1
/* INTSEL, SOCASEL, SOCBSEL bits */
#define ET_CTR_ZERO      0x1
#define ET_CTR_PRD       0x2
#define ET_CTRU_CMPA     0x4
#define ET_CTRD_CMPA     0x5
#define EC_CTRU_CMPB     0x6
#define EC_CTRD_CMPB     0x7

/* ETPS (Event-trigger Prescale) */

/* INTPRD, SOCAPRD, SOCBPRD bits */
#define ET_DISABLE       0x0
#define ET_1ST           0x1
#define ET_2ND           0x2
#define ET_3RD           0x3

/* HRCNFG (High-resolution PWM configuration) */

/* EDGMODE */
#define HR_MEP_DISABLE   0x0
#define HR_MEP_RISING    0x1
#define HR_MEP_FALLING   0x2
#define HR_MEP_BOTH      0x3
/* CTLMODE */
#define HR_DUTY          0x0
#define HR_PHASE         0x1
/* HRLOAD */
#define HR_CTR_ZERO      0x0
#define HR_CTR_PRD       0x1

#define NCHAN 2

struct ehrpwm_platform_data {
	int channel_mask;
};

struct ehrpwm_pwm;

typedef int (*p_fcallback) (struct ehrpwm_pwm *, void *data);

struct et_int {
	void *data;
	p_fcallback pcallback;
};

struct tz_int {
	void *data;
	p_fcallback pcallback;
};

struct ehrpwm_pwm {
	struct pwm_device pwm[NCHAN];
	struct pwm_device_ops ops;
	spinlock_t      lock;
	struct clk      *clk;
	void __iomem    *mmio_base;
	unsigned short prescale_val;
	int irq[2];
	struct et_int st_etint;
	struct tz_int st_tzint;
};

enum tz_event {
	TZ_ONE_SHOT_EVENT = 0,
	TZ_CYCLE_BY_CYCLE,
	TZ_OSHT_CBC,
	TZ_DIS_EVT,
};

enum config_mask {
	CONFIG_NS,
	CONFIG_TICKS,
};

enum db_edge_delay {
	RISING_EDGE_DELAY,
	FALLING_EDGE_DELAY,
};

struct aq_config_params {
	unsigned char ch;
	unsigned char ctreqzro;
	unsigned char ctreqprd;
	unsigned char ctreqcmpaup;
	unsigned char ctreqcmpadown;
	unsigned char ctreqcmpbup;
	unsigned char ctreqcmpbdown;
};

int ehrpwm_tb_set_prescalar_val(struct pwm_device *p, unsigned char
	clkdiv, unsigned char hspclkdiv);

int ehrpwm_tb_config_sync(struct pwm_device *p, unsigned char phsen,
	unsigned char syncosel);

int ehrpwm_tb_set_counter_mode(struct pwm_device *p, unsigned char
	ctrmode, unsigned char phsdir);

int ehrpwm_tb_force_sync(struct pwm_device *p);

int ehrpwm_tb_set_periodload(struct pwm_device *p, unsigned char
	       loadmode);

int ehrpwm_tb_set_counter(struct pwm_device *p, unsigned short val);

int ehrpwm_tb_read_status(struct pwm_device *p, unsigned short *val);

int ehrpwm_tb_read_counter(struct pwm_device *p, unsigned short *val);

int ehrpwm_tb_set_period(struct pwm_device *p,	unsigned short val);

int ehrpwm_tb_set_phase(struct pwm_device *p, unsigned short val);

int ehrpwm_cmp_set_cmp_ctl(struct pwm_device *p, unsigned char
	       shdwamode, unsigned char shdwbmode, unsigned char loadamode,
	unsigned char loadbmode);

int ehrpwm_cmp_set_cmp_val(struct pwm_device *p, unsigned char reg,
	unsigned short val);

int ehrpwm_aq_set_act_ctrl(struct pwm_device *p,
	       struct aq_config_params *cfg);

int ehrpwm_aq_set_one_shot_act(struct pwm_device  *p, unsigned char ch,
	unsigned char act);

int ehrpwm_aq_ot_frc(struct pwm_device *p, unsigned char ch);

int ehrpwm_aq_set_csfrc_load_mode(struct pwm_device *p, unsigned char
	       loadmode);

int ehrpwm_aq_continuous_frc(struct pwm_device *p, unsigned char ch,
	unsigned char act);

int ehrpwm_db_get_max_delay(struct pwm_device *p,
	       enum config_mask cfgmask, unsigned long *delay_val);

int ehrpwm_db_get_delay(struct pwm_device *p, unsigned char edge,
	enum config_mask cfgmask, unsigned long *delay_val);

int ehrpwm_db_set_delay(struct pwm_device *p, unsigned char edge,
		enum config_mask cfgmask, unsigned long delay);

int ehrpwm_db_set_mode(struct pwm_device *p, unsigned char inmode,
		unsigned char polsel, unsigned char outmode);

int ehrpwm_pc_configure(struct pwm_device *p, unsigned char chpduty,
		unsigned char chpfreq, unsigned char oshtwidth);

int ehrpwm_pc_en_dis(struct pwm_device *p, unsigned char chpen);

int ehrpwm_tz_sel_event(struct pwm_device *p, unsigned char input,
	       enum tz_event evt);

int ehrpwm_tz_set_action(struct pwm_device *p, unsigned char ch,
	unsigned char act);

int ehrpwm_tz_set_int_en_dis(struct pwm_device *p, enum tz_event event,
		unsigned char int_en_dis);

int ehrpwm_tz_force_evt(struct pwm_device *p, enum tz_event event);

int ehrpwm_tz_read_status(struct pwm_device *p, unsigned short *status);

int ehrpwm_tz_clr_evt_status(struct pwm_device *p);

int ehrpwm_tz_clr_int_status(struct pwm_device *p);

int ehrpwm_et_set_sel_evt(struct pwm_device *p, unsigned char evt,
		unsigned char prd);

int ehrpwm_et_int_en_dis(struct pwm_device *p, unsigned char en_dis);

int ehrpwm_et_read_evt_cnt(struct pwm_device *p, unsigned long *evtcnt);

int pwm_et_read_int_status(struct pwm_device *p,
	unsigned long *status);

int ehrpwm_et_frc_int(struct pwm_device *p);

int ehrpwm_et_clr_int(struct pwm_device *p);

int ehrpwm_hr_set_phase(struct pwm_device *p, unsigned char val);

int ehrpwm_hr_set_cmpval(struct pwm_device *p, unsigned char val);

int ehrpwm_hr_config(struct pwm_device *p, unsigned char loadmode,
		unsigned char ctlmode, unsigned char edgemode);

int ehrpwm_et_cb_register(struct pwm_device *p, void *data,
	p_fcallback cb);

int ehrpwm_tz_cb_register(struct pwm_device *p, void *data,
	p_fcallback cb);

int ehrpwm_pwm_suspend(struct pwm_device *p, enum
	       config_mask config_mask,
	       unsigned long val);

#endif
