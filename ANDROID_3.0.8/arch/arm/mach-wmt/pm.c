/*++
linux/arch/arm/mach-wmt/pm.c

Copyright (c) 2008  WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software Foundation,
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/
#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/sysctl.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

#include <mach/hardware.h>
#include <asm/memory.h>
#include <asm/system.h>
#include <asm/leds.h>
#include <asm/io.h>
#include <linux/rtc.h>

#include <asm/cacheflush.h>
#include <asm/hardware/cache-l2x0.h>


/*kevin add for debug pmw
	1 init_trace_pmw(); to init
	2 TRACE_PMW; in where you want
	3  print_trace_pmw(); print result

*/
int pmw_debug_index = 0;
unsigned char pmw_debug[100][200];
#define TRACE_PMW	do{					\
	sprintf(pmw_debug[pmw_debug_index],"%s %d 0x%x 0x%x\n",__FUNCTION__,__LINE__,PMWE_VAL,PMWT_VAL);	\
	pmw_debug_index++;			\
}while(0)


void init_trace_pmw(void){
	memset(pmw_debug,0,100*200);
	pmw_debug_index = 0;
}

void print_trace_pmw(){
	int i;
	for(i=0;i<100;i++){
		if(strlen(pmw_debug[i])>0)
			printk(pmw_debug[i]);
	}
}

//#define CONFIG_KBDC_WAKEUP
//#define KB_WAKEUP_SUPPORT
//#define MOUSE_WAKEUP_SUPPORT
//#define ETH_WAKEUP_SUPPORT
#define	SOFT_POWER_SUPPORT
#define	RTC_WAKEUP_SUPPORT
#ifdef CONFIG_RMTCTL_WonderMedia //define if CIR menuconfig enable
     #define CIR_WAKEUP_SUPPORT
#endif
#define KEYPAD_POWER_SUPPORT
#define PMWT_C_WAKEUP(src, type)  ((type & PMWT_TYPEMASK) << (((src - 24) & PMWT_WAKEUPMASK) * 4))

enum wakeup_src_e {
	WKS_SRC0        = 0,     /* General Purpose Wakeup Source 0          */
	WKS_SRC1        = 1,     /* General Purpose Wakeup Source 1          */
	WKS_SRC2        = 2,     /* General Purpose Wakeup Source 2          */
	WKS_SRC3        = 3,     /* General Purpose Wakeup Source 3          */
	WKS_SRC4        = 4,     /* General Purpose Wakeup Source 4          */
	WKS_SRC5        = 5,     /* General Purpose Wakeup Source 5          */
	WKS_SRC6        = 6,     /* General Purpose Wakeup Source 6          */
	WKS_SRC7        = 7,     /* General Purpose Wakeup Source 7          */
	WKS_RTC         = 15,    /* RTC alarm interrupt as wakeup            */
	WKS_ETH         = 17,    /* ETH interrupt as wakeup                  */
	WKS_UHC         = 20,    /* UHC interrupt as wakeup                  */
	WKS_UDC         = 21,    /* UDC interrupt as wakeup                  */
	WKS_CIR         = 22,    /* CIR interrupt as wakeup                  */
	WKS_CF          = 26,    /* CF  interrupt as wakeup                  */
	WKS_XD,                  /* XD  interrupt as wakeup                  */
	WKS_MS,                  /* MS  interrupt as wakeup                  */
	WKS_SD0,                 /* SD0 interrupt as wakeup                  */
	WKS_SD1,                 /* SD1 interrupt as wakeup                  */
	WKS_SC,                  /* SmartCard interrupt as wakeup            */
	WKS_NUM                  /* Wakeup event number                      */
};

#define DRIVER_NAME	"PMC"
#if defined(CONFIG_PM_RTC_IS_GMT) && defined(RTC_WAKEUP_SUPPORT)
#include <linux/rtc.h>
#endif

/*
 *  Debug macros
 */
#ifdef DEBUG
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)
#else
#  define DPRINTK(fmt, args...)
#endif

/*
 * For saving discontinuous registers during hibernation.
 */
#define SAVE(x)		(saved[SAVED_##x] = (x##_VAL))
#define RESTORE(x)	((x##_VAL) = saved[SAVED_##x])

enum {
	SAVED_SP = 0,
	SAVED_OSTW, SAVED_OSTI,
	SAVED_PMCEL, SAVED_PMCEU,
	SAVED_PMCE2, SAVED_PMCE3,
	/* SAVED_ATAUDMA, */
	SAVED_SIZE
};

struct apm_dev_s {
    char id;
};


extern unsigned int wmt_read_oscr(void);
extern void wmt_read_rtc(unsigned int *date, unsigned int *time);

// import extern function for usb-charge
extern int wmt_batt_charge_type(void);
extern int wmt_read_dcin_state(void);


/* Hibernation entry table physical address */
#define LOADER_ADDR												0xffff0000
#define HIBERNATION_ENTER_EXIT_CODE_BASE_ADDR	0xFFFFFFC0
#define DO_POWER_ON_SLEEP			            (HIBERNATION_ENTER_EXIT_CODE_BASE_ADDR + 0x00)
#define DO_POWER_OFF_SUSPEND			        (HIBERNATION_ENTER_EXIT_CODE_BASE_ADDR + 0x04)
#define DO_WM_IO_SET							        (HIBERNATION_ENTER_EXIT_CODE_BASE_ADDR + 0x34)

static unsigned int exec_at = (unsigned int)-1;

/*from = 4 high memory*/
static void (*theKernel)(int from);
//static void (*theKernel_io)(int from);

#if defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)
//static struct proc_dir_entry *proc_softpower;
static unsigned int softpower_data;
#endif

static long rtc2sys;

struct work_struct	PMC_shutdown;
struct work_struct	PMC_sync;

#define GPIO_PHY_BASE_ADDR (GPIO_BASE_ADDR-WMT_MMAP_OFFSET)
#define ENV_GPIO_CHARGE_ELEC "wmt.gpo.charge_elec"
struct gpio_operation_t {
	unsigned int id;
	unsigned int act;
	unsigned int bitmap;
	unsigned int ctl;
	unsigned int oc;
	unsigned int od;
};
static struct gpio_operation_t s_charge_elec_pin;

extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
static int power_on_debounce_value = 100;  /*power button debounce time when power on state*/
static int resume_debounce_value = 2000;	/*power button debounce time when press button to resume system*/
static int power_up_debounce_value = 2000; /*power button debounce time when press button to power up*/
#define min_debounce_value 0
#define max_debounce_value 4000

char hotplug_path[256] = "/sbin/hotplug";
static unsigned int sync_counter = 0;
static unsigned int time1, time2;

#define REG_VAL(addr) (*((volatile unsigned int *)(addr)))


#ifdef KEYPAD_POWER_SUPPORT
#include <linux/input.h>
#define KPAD_POWER_FUNCTION_NUM  1
#define power_button_timeout (HZ/10)
static struct input_dev *kpadPower_dev;
static unsigned int kpadPower_codes[KPAD_POWER_FUNCTION_NUM] = {
        [0] = KEY_POWER
};
static unsigned int powerKey_is_pressed;
static unsigned int pressed_jiffies;
static struct timer_list   kpadPower_timer;
static spinlock_t kpadPower_lock;
#endif

#ifdef ETH_WAKEUP_SUPPORT
static unsigned int eth;
#endif

#ifdef CONFIG_BATTERY_WMT
static unsigned int battery_used;
#endif

#ifdef CONFIG_CACHE_L2X0
unsigned int l2x0_onoff;
unsigned int l2x0_aux;
unsigned int l2x0_prefetch_ctrl;
#endif

//gri
static unsigned int var_wake_type=0;
static unsigned int var_wake_param=0;
static unsigned int var_wake_en=0;
//kevin 
static unsigned int var_wake_type_inresume=0;
static unsigned int var_wake_param_inresume=0;
static unsigned int var_wake_en_inresume=0;


unsigned int WMT_WAKE_UP_EVENT;

static int wmt_cir_enable = 0;

/* wmt_pwrbtn_debounce_value()
 *
 * Entry to set the power button debounce value, the time unit is ms.
 */
static void wmt_pwrbtn_debounce_value(unsigned int time)
{
	volatile unsigned long debounce_value = 0 ;
	unsigned long pmpb_value = 0;

	/*add a delay to wait pmc & rtc  sync*/
	udelay(100);
	
	/*Debounce value unit is 1024 * RTC period ,RTC is 32KHz so the unit is ~ 32ms*/
	if (time % 32)
		debounce_value = (time / 32) + 1;
	else
		debounce_value = (time / 32);

	pmpb_value = PMPB_VAL;
	pmpb_value &= ~ PMPB_DEBOUNCE(0xff);
	pmpb_value |= PMPB_DEBOUNCE(debounce_value);

	PMPB_VAL = pmpb_value;
	//udelay(100);
	DPRINTK("[%s] PMPB_VAL = 0x%.8X \n",__func__,PMPB_VAL);
}

/* wmt_power_up_debounce_value()
 *
 * Entry to set the power button debounce value, the time unit is ms.
 */
void wmt_power_up_debounce_value(void) {

	//printk("[%s] power_up_debounce_value = %d \n",__func__,power_up_debounce_value);
	wmt_pwrbtn_debounce_value(power_up_debounce_value);

}

static void run_sync(struct work_struct *work)
{
	int ret;
  char *argv[] = { "/system/etc/wmt/script/force.sh", "PMC", NULL };
	char *envp_shutdown[] =
	{ "HOME=/", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", "", NULL };

	wmt_pwrbtn_debounce_value(power_up_debounce_value);
	DPRINTK("[%s] start\n",__func__);
	ret = call_usermodehelper(argv[0], argv, envp_shutdown, 0);
	DPRINTK("[%s] sync end\n",__func__);
}

static void run_shutdown(struct work_struct *work)
{
	int ret;
    char *argv[] = { hotplug_path, "PMC", NULL };
	char *envp_shutdown[] =
		{ "HOME=/", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", "ACTION=shutdown", NULL };
	DPRINTK("[%s] \n",__func__);
   
	wmt_pwrbtn_debounce_value(power_up_debounce_value);
	ret = call_usermodehelper(argv[0], argv, envp_shutdown, 0);
}

/* Blink program led by Dedicated GPIO9 */
#if 0
static void led_light(unsigned int count)
{
    unsigned int result = 0;
    while (count-- > 0) {
        REG_VAL(0xD8110024) |= 0x80;
        REG_VAL(0xD8110054) |= 0x80;
        REG_VAL(0xD8110084) &= ~0x80;

        for (result = 3000000; result > 0; result--)
            REG_VAL(0xD8110084) &= ~0x80;
        for (result = 3000000; result > 0; result--)
            REG_VAL(0xD8110084) |= 0x80;        
    }
}
#endif

irqreturn_t viatelcom_irq_cp_wake_ap(int irq, void *data);
static irqreturn_t pmc_wakeup_isr(int this_irq, void *dev_id)
{
	unsigned int status;
	unsigned long flags;

	status = PMWS_VAL;          /* Copy the wakeup status */
	udelay(100);
	PMWS_VAL = status;
	//printk("pmc_wakeup_isr %x\n",status);

	if(status & BIT1){
		viatelcom_irq_cp_wake_ap(this_irq,dev_id);
		PMWS_VAL |= BIT1;
		
	}
	
#ifdef KEYPAD_POWER_SUPPORT
	if((status & BIT14) && kpadPower_dev) {

		spin_lock_irqsave(&kpadPower_lock, flags);
		if(!powerKey_is_pressed) {
			powerKey_is_pressed = 1; 
			input_report_key(kpadPower_dev, KEY_POWER, 1); //power key is pressed
			input_sync(kpadPower_dev);
			pressed_jiffies = jiffies;
			wmt_pwrbtn_debounce_value(power_up_debounce_value);
			DPRINTK("\n[%s]power key pressed -->\n",__func__);
			time1 = jiffies_to_msecs(jiffies);
		} else {
			input_event(kpadPower_dev, EV_KEY, KEY_POWER, 2); // power key repeat
			input_sync(kpadPower_dev);
			DPRINTK("\n[%s]power key repeat\n",__func__);

		}
		//disable_irq(IRQ_PMC_WAKEUP);
		spin_unlock_irqrestore(&kpadPower_lock, flags);
		mod_timer(&kpadPower_timer, jiffies + power_button_timeout);
	}
#endif


    
	if (status & BIT14) {       /* Power button wake up */
#if defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)
		softpower_data = 1;
#endif
		/*schedule_work(&PMC_shutdown);*/
	}
	
	if (status & (1 << WKS_UHC)) {       /* UHC wake up */
		PMWE_VAL &= ~(1 << WKS_UHC);
	}

#ifdef RTC_WAKEUP_SUPPORT
	if (status & BIT15)        /* Check RTC wakeup status bit */
		PMWS_VAL |= BIT15;
#endif

#ifdef MOUSE_WAKEUP_SUPPORT
	if(status & BIT11)
		PMWS_VAL |= BIT11;
#endif

#ifdef KB_WAKEUP_SUPPORT
	if(status & BIT11)
		PMWS_VAL |= BIT10;
#endif

	return IRQ_HANDLED;
}

extern int PM_device_PostSuspend(void);
extern int PM_device_PreResume(void);

void check_pmc_busy(void)
{
	while (PMCS2_VAL&0x3F0038)
	;
}
void save_plla_speed(unsigned int *plla_div)
{
	plla_div[0] = PMARM_VAL;/*arm_div*/
	check_pmc_busy();
	plla_div[1]= PML2C_VAL;;/*l2c_div*/
	check_pmc_busy();
	plla_div[2] = PML2CTAG_VAL;/*l2c_tag_div*/
	check_pmc_busy();
	plla_div[3] = PML2CDATA_VAL;/*l2c_data_div*/
	check_pmc_busy();
	plla_div[4] = PML2CAXI_VAL;/*axi_l2c_div*/
	check_pmc_busy();
	plla_div[5] = PMDBGAPB_VAL;/*dbg_apb_div*/
	check_pmc_busy();
	plla_div[6] = PMPMA_VAL;
	check_pmc_busy();
	plla_div[7] = PMNA12_VAL;
	check_pmc_busy();
	plla_div[8] = PMMALI_VAL;
	check_pmc_busy();
	plla_div[9] = PMAHB_VAL;
	check_pmc_busy();
	plla_div[10] = PMAPB0_VAL;
	check_pmc_busy();
	plla_div[11] = PMPMB_VAL;
}

void restore_plla_speed(unsigned int *plla_div)
{

	auto_pll_divisor(DEV_ARM, SET_PLLDIV, 2, 300);
	PMARM_VAL = plla_div[0];/*arm_div*/
	check_pmc_busy();
	PML2C_VAL = plla_div[1];/*l2c_div*/
	check_pmc_busy();
	PML2CTAG_VAL = plla_div[2];/*l2c_tag_div*/
	check_pmc_busy();
	PML2CDATA_VAL = plla_div[3];/*l2c_data_div*/
	check_pmc_busy();
	PML2CAXI_VAL = plla_div[4];/*l2c_axi_div*/
	check_pmc_busy();
	PMDBGAPB_VAL = plla_div[5];/*dbg_apb_div*/
	check_pmc_busy();
	PMPMA_VAL = plla_div[6];
	check_pmc_busy();
	PMNA12_VAL = plla_div[7];
	check_pmc_busy();
	PMMALI_VAL = plla_div[8];
	check_pmc_busy();
	PMAHB_VAL = plla_div[9];
	check_pmc_busy();
	PMAPB0_VAL = plla_div[10];
	check_pmc_busy();
	PMPMB_VAL = plla_div[11];
	check_pmc_busy();
}

/* wmt_pm_standby()
 *
 * Entry to the power-on sleep hibernation mode.
 */
static void wmt_pm_standby(void)
{
	volatile unsigned int hib_phy_addr = 0,base = 0;    

#ifdef CONFIG_CACHE_L2X0
	volatile unsigned int l2x0_base;
	__u32 power_cntrl;
	
	if( l2x0_onoff == 1)
	{
		l2x0_base = (volatile unsigned int) ioremap(0xD9000000, SZ_4K);
		
		outer_cache.flush_all();
		outer_cache.disable();
		outer_cache.inv_all();
	}
#endif

	/* Get standby virtual address entry point */    
	base = (unsigned int)ioremap_nocache(LOADER_ADDR, 0x10000);

	exec_at = base + (DO_POWER_ON_SLEEP - LOADER_ADDR);
	hib_phy_addr = *(unsigned int *) exec_at;
	exec_at = base + (hib_phy_addr - LOADER_ADDR);

	//led_light(3);
	theKernel = (void (*)(int))exec_at;		/* set rom address */
	theKernel(4);					        /* jump to rom */
	//led_light(3);

	iounmap((void __iomem *)base);

#ifdef CONFIG_CACHE_L2X0
	if( l2x0_onoff == 1)
	{
		if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & 1))
		{
			/* l2x0 controller is disabled */
			writel_relaxed(l2x0_aux, l2x0_base + L2X0_AUX_CTRL);

			writel_relaxed(0x110, l2x0_base + L2X0_TAG_LATENCY_CTRL);
			writel_relaxed(0x110, l2x0_base + L2X0_DATA_LATENCY_CTRL);
			power_cntrl = readl_relaxed(l2x0_base + L2X0_POWER_CTRL) | L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN;
			writel_relaxed(power_cntrl, l2x0_base + L2X0_POWER_CTRL);

			writel_relaxed(l2x0_prefetch_ctrl, l2x0_base + L2X0_PREFETCH_CTRL);

			outer_cache.inv_all();

			/* enable L2X0 */
			writel_relaxed(1, l2x0_base + L2X0_CTRL);
		}
	
		iounmap((void __iomem *)l2x0_base);
	}
#endif
}

//largeElec = 1: large electricity charge,  largeElec = 0: small electricity charge
void pull_charge_elec_pin(int largeElec)
{
	unsigned int val;

	if ((s_charge_elec_pin.ctl == 0) || (s_charge_elec_pin.oc == 0) || (s_charge_elec_pin.od == 0)) 
		return;

	val = 1 << s_charge_elec_pin.bitmap;
	
	if (s_charge_elec_pin.act == 0)
		largeElec = ~largeElec;

	if (largeElec) {
		REG8_VAL(s_charge_elec_pin.od) |= val;
	} else {
		REG8_VAL(s_charge_elec_pin.od) &= ~val;
	}

	REG8_VAL(s_charge_elec_pin.oc) |= val;
	REG8_VAL(s_charge_elec_pin.ctl) |= val; 

}
EXPORT_SYMBOL(pull_charge_elec_pin);

extern void wmt_assem_suspend(void);

/* wmt_pm_suspend()
 *
 * Entry to the power-off suspend hibernation mode.
 */
 //int oem_gpio_direction_output(int gpio, int value);
static void wmt_pm_suspend(void)
{
	unsigned int saved[SAVED_SIZE];
	int result;
	unsigned int plla_div[12];

	volatile unsigned int hib_phy_addr = 0,base = 0;

#ifdef CONFIG_CACHE_L2X0
	volatile unsigned int l2x0_base;
	__u32 power_cntrl;
#endif

/* FIXME */
#if 1
	result = PM_device_PostSuspend();
	if (result)
		printk("PM_device_PostSuspend fail\n");
#endif

	if (wmt_batt_charge_type() == 1) {
		// do nothing, keep old status
	}
	else
		pull_charge_elec_pin(1);

#ifdef CONFIG_CACHE_L2X0
	if( l2x0_onoff == 1)
	{
		l2x0_base = (volatile unsigned int) ioremap(0xD9000000, SZ_4K);
		
		outer_cache.flush_all();
		outer_cache.disable();
		outer_cache.inv_all();
	}
#endif

	SAVE(OSTW);	                /* save vital registers */
	SAVE(OSTI);	    
	SAVE(PMCEL);	            /* save clock gating */
	SAVE(PMCEU);
	SAVE(PMCE2);
	SAVE(PMCE3);
	
	//PMWE_VAL |= 0x1; //enable wake-up 0 for low power wake up
	//mdelay(500);
	
	base = (unsigned int)ioremap_nocache(LOADER_ADDR, 0x10000);

	/* Get suspend virtual address entry point */        
	exec_at = base + (DO_POWER_OFF_SUSPEND - LOADER_ADDR);
	hib_phy_addr = *(unsigned int *) exec_at;
	exec_at = base + (hib_phy_addr - LOADER_ADDR);

	save_plla_speed(plla_div);
	//led_light(2);
	
//	theKernel = (void (*)(int))exec_at; 
//	theKernel(4);
	wmt_assem_suspend();

	/* do wm_io_set in w-loader*/
//	exec_at = base + (DO_WM_IO_SET - LOADER_ADDR);

//	theKernel_io = (void (*)(int))exec_at;
//	theKernel_io(4);
	//led_light(2);

	iounmap((void __iomem *)base);
	
	RESTORE(PMCE3);
	RESTORE(PMCE2);
	RESTORE(PMCEU);             /* restore clock gating */
	RESTORE(PMCEL);             
	RESTORE(OSTI);              /* restore vital registers */
	RESTORE(OSTW);
	restore_plla_speed(plla_div);	/* restore plla clock register */

	PMPB_VAL |= 1;				/* cant not clear RGMii connection */
//	PMWE_VAL |= 3; //simen add for enable wakeup 0/1
#ifdef CONFIG_CACHE_L2X0
	if( l2x0_onoff == 1)
	{
		if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & 1))
		{
			/* l2x0 controller is disabled */
			writel_relaxed(l2x0_aux, l2x0_base + L2X0_AUX_CTRL);

			writel_relaxed(0x110, l2x0_base + L2X0_TAG_LATENCY_CTRL);
			writel_relaxed(0x110, l2x0_base + L2X0_DATA_LATENCY_CTRL);
			power_cntrl = readl_relaxed(l2x0_base + L2X0_POWER_CTRL) | L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN;
			writel_relaxed(power_cntrl, l2x0_base + L2X0_POWER_CTRL);

			writel_relaxed(l2x0_prefetch_ctrl, l2x0_base + L2X0_PREFETCH_CTRL);

  			outer_cache.inv_all(); 

			/* enable L2X0 */
			writel_relaxed(1, l2x0_base + L2X0_CTRL);
		}
	
		iounmap((void __iomem *)l2x0_base);
	}
#endif

	if (wmt_batt_charge_type() == 1) {
		// do nothing, keep old status
	}
	else
		pull_charge_elec_pin(0);

	result = PM_device_PreResume();
	if (!result)
		printk("PM_device_PreResume fail\n");

	//printk("========================================system resume\n");
	//oem_gpio_direction_output(1,1);


	
}

/* wmt_pm_enter()
 *
 * To Finally enter the sleep state.
 *
 * Note: Only support PM_SUSPEND_STANDBY and PM_SUSPEND_MEM
 */
unsigned int pmc_wake_sts; 
static int wmt_pm_enter(suspend_state_t state)
{
	char value[32] = {'\0',};
	int len = 31;
	unsigned int status;
	//unsigned int wakeup_notify;
	volatile unsigned int Wake_up_enable = 0;
	volatile unsigned int Wake_up_type = 0 /*, Wake_up_Ctype = 0*/;
    
	if (!((state == PM_SUSPEND_STANDBY) || (state == PM_SUSPEND_MEM))) {
		printk(KERN_ALERT "%s, Only support PM_SUSPEND_STANDBY and PM_SUSPEND_MEM\n", DRIVER_NAME);
		return -EINVAL;
	}

/*prepare wake up source*/
	if (var_wake_en){
		
#ifdef RTC_WAKEUP_SUPPORT
		if (var_wake_param & BIT15)
			Wake_up_enable |= (PMWE_RTC);    
#endif

#ifdef CONFIG_KBDC_WAKEUP
		i8042_write_2e(0xE0);
		i8042_write_2f(0xB);

#ifdef MOUSE_WAKEUP_SUPPORT
		Wake_up_enable |= (BIT11);
		i8042_write_2e(0xE9);
		i8042_write_2f(0x00);
#endif
#ifdef KB_WAKEUP_SUPPORT
		Wake_up_enable |= (BIT10);
		i8042_write_2e(0xE1);
		i8042_write_2f(0x00);
#endif
#endif

#if 0	
		/*UDC wake up source*/
		if (var_wake_param & BIT21){
			Wake_up_enable |= (1<<WKS_UDC);    
			Wake_up_type   |= PMWT_WAKEUP(WKS_UDC,PMWT_RISING);
		}
#endif

#if 0
	/*CF wake up source*/
		Wake_up_enable |= (1<<WKS_CF);    
		Wake_up_Ctype   |= PMWT_WAKEUP(WKS_CF,PMWT_EDGE);
#endif
	/*
	 * choose wake up event
	 */
#if 0   
	//UHC  
		if (var_wake_param & BIT20){
			if (state == PM_SUSPEND_STANDBY)                      /* PM_SUSPEND_MEM */
			{
				Wake_up_enable |= (1 << WKS_UHC);
				Wake_up_type |= PMWT_WAKEUP(WKS_UHC,PMWT_ONE);
			} else {                                              /* PM_SUSPEND_MEM */
				Wake_up_enable |= (1 << WKS_UHC);
				Wake_up_type |= PMWT_WAKEUP(WKS_UHC,PMWT_ONE);
			}
		}
#endif
#ifdef CIR_WAKEUP_SUPPORT   
		if (var_wake_param & BIT22){  
			if (state == PM_SUSPEND_STANDBY)                      /* PM_SUSPEND_MEM */
			{
				Wake_up_enable |= (1 << WKS_CIR);
				Wake_up_type   |= PMWT_C_WAKEUP(WKS_CIR,PMWT_RISING);
			} else {                                              /* PM_SUSPEND_MEM */
				Wake_up_enable |= (1 << WKS_CIR);
				Wake_up_type   |= PMWT_C_WAKEUP(WKS_CIR,PMWT_RISING);
			}
		}
#endif
#ifdef CONFIG_BATTERY_WMT
		if (var_wake_param & BIT0){  
			if(battery_used){
				unsigned int temp_type;
				if (state == PM_SUSPEND_STANDBY)                      /* PM_SUSPEND_MEM */
				{
					Wake_up_enable |= (1 << WKS_SRC0);
					temp_type = (var_wake_type & 0xf) >> 0;
					Wake_up_type   |= PMWT_C_WAKEUP(WKS_SRC0,temp_type);
				} else {                                              /* PM_SUSPEND_MEM */
					Wake_up_enable |= (1 << WKS_SRC0);
					temp_type = (var_wake_type & 0xf) >> 0;
					Wake_up_type   |= PMWT_C_WAKEUP(WKS_SRC0,temp_type);
				}
			}
		}
#endif

		if(var_wake_param & BIT3){//gri
			unsigned int temp_type;
			if (state == PM_SUSPEND_STANDBY)                      /* PM_SUSPEND_MEM */
			{
				Wake_up_enable |= (1 << WKS_SRC3);
				temp_type = (var_wake_type & 0xf000) >> 12;
				Wake_up_type   |= PMWT_C_WAKEUP(WKS_SRC3,temp_type);
			} else {                                              /* PM_SUSPEND_MEM */
				Wake_up_enable |= (1 << WKS_SRC3);
				temp_type = (var_wake_type & 0xf000) >> 12;
				Wake_up_type   |= PMWT_C_WAKEUP(WKS_SRC3,temp_type);
			}
		}


#ifdef ETH_WAKEUP_SUPPORT
		//if((eth & 0x90) == 0x90) {
		if(((eth & 0x10) == 0x10) && (var_wake_param & BIT17)) {
			//enable Wake On Lan
			Wake_up_enable |= (1 << WKS_ETH);
			Wake_up_type   |= PMWT_C_WAKEUP(WKS_ETH, PMWT_ONE);
		} else {
			//disable Wake On Lan
			//poweroff RGMii
			PMPB_VAL &= 0xfffffffd;
		}
#endif
	}

	if (wmt_batt_charge_type() == 1) {
		// wakeup system when usb hotplug, wakeup source is GPIO[wakeup2]
		
		if (wmt_read_dcin_state() == 0) {
			// no charge, GPIO[wakeup2] is high level, set to falling-edge triggle
			Wake_up_type   |= PMWT_C_WAKEUP(WKS_SRC2, PMWT_FALLING);
		}
		else {
			// charging, GPIO[wakeup2] is low level, set to rising-edge triggle
			Wake_up_type   |= PMWT_C_WAKEUP(WKS_SRC2, PMWT_RISING);
		}
		
		Wake_up_enable |= (1 << WKS_SRC2);

		// Note: we don't use UDC wakeup, because of some issue in UDC drive???
		//Wake_up_enable |= (1 << WKS_UDC);
	}

	/* only enable fiq in normal operation */
	local_fiq_disable();
	//local_irq_disable();  //do it in arch_suspend_disable_irqs() which is called by suspend_enter() in suspend.c
	/* disable system OS timer */
	OSTC_VAL &= ~OSTC_ENABLE;

	/*wake up status W1C*/
	PMWS_VAL = PMWS_VAL;     
	//WMT_WAKE_UP_EVENT = 0;
	/* FIXME, 2009/09/15 */
	//PMCDS_VAL = PMCDS_VAL;

	/* 2008/09/24 Neil
	 * Write to Wake-up Event Enable Register ONLY once, it is becausebe this register is 
	 * synced into RTC clock domain. Which means two consequtive write cannot be too close
	 * (wait more than 2~3 RTC clocks). Otherwise the later write data will be missing.
	 */
	if (wmt_cir_enable) {
		// if cir enable, dont set GPIO_PIN_SHARING_SEL_4BYTE_VAL[25]
		Wake_up_enable |= 0x1;
	}
	else {
		GPIO_PIN_SHARING_SEL_4BYTE_VAL |= 0x2000000;
		Wake_up_enable |= 0x3;
	}
	
	//kevin add ,wait 200ms for pre wakeup register writting to take effort
	udelay(200);
	if (wmt_batt_charge_type() == 1) {
		// wakeup system when usb hotplug, wakeup source is GPIO[wakeup2]
		
		if (wmt_read_dcin_state() == 0) {
			// no charge, GPIO[wakeup2] is high level, set to falling-edge triggle
			PMWT_VAL = (var_wake_type | PMWT_C_WAKEUP(WKS_SRC2, PMWT_FALLING));
		}
		else {
			// charging, GPIO[wakeup2] is low level, set to rising-edge triggle
			PMWT_VAL = (var_wake_type | PMWT_C_WAKEUP(WKS_SRC2, PMWT_RISING));
		}
		
		Wake_up_enable = (var_wake_param | (1 << WKS_SRC2));
	}
	else {
		PMWT_VAL = var_wake_type;    
		Wake_up_enable = var_wake_param;
	}

	if (wmt_cir_enable)
		Wake_up_enable |= (1 << WKS_CIR);

	PMWE_VAL = Wake_up_enable;
	
	//sprintf(nouart_buf[nouart_index++],"%s %d %x %x\n",__FUNCTION__,__LINE__,PMWT_VAL,PMWE_VAL);
	
	/*set power button debounce value*/
	wmt_pwrbtn_debounce_value(resume_debounce_value);

	/*
	 * We use pm_standby as apm_standby for power-on hibernation.
	 * but we still suspend memory in both case.
	 */



	if (state == PM_SUSPEND_STANDBY)
		wmt_pm_standby();    /* Go to standby mode*/
	else
		wmt_pm_suspend();    /* Go to suspend mode*/

	/*if (wmt_batt_charge_type() == 1) {
		// wakeup system when usb hotplug, wakeup source is GPIO[wakeup2]
		
		if (wmt_read_dcin_state() == 0) {
			// no charge, GPIO[wakeup2] is high level, set to falling-edge triggle
			PMWT_VAL = (var_wake_type | PMWT_C_WAKEUP(WKS_SRC2, PMWT_FALLING));
		}
		else {
			// charging, GPIO[wakeup2] is low level, set to rising-edge triggle
			PMWT_VAL = (var_wake_type | PMWT_C_WAKEUP(WKS_SRC2, PMWT_RISING));
		}
		
		PMWE_VAL = (var_wake_param | (1 << WKS_SRC2));
	}
	else {
		PMWT_VAL = var_wake_type;    
		PMWE_VAL = var_wake_param;
	}*/		
	
	PMWT_VAL = var_wake_type_inresume;    
	PMWE_VAL = var_wake_param_inresume;

	/*set power button debounce value*/
	wmt_pwrbtn_debounce_value(power_on_debounce_value);

	/*
	 * Clean wakeup source
	 */

	status = PMWS_VAL;
	WMT_WAKE_UP_EVENT = PMWS_VAL;
	udelay(100);
	PMWS_VAL = status;
	pmc_wake_sts = status;

//--> removed by howayhuo on 20120402
#if 0 

#ifdef KEYPAD_POWER_SUPPORT	
	if (status & BIT14) {
		DPRINTK(KERN_ALERT "\n[%s]power key pressed\n",__func__);
		powerKey_is_pressed = 1; 
		input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
		input_sync(kpadPower_dev);
		pressed_jiffies = jiffies;
		mod_timer(&kpadPower_timer, jiffies + power_button_timeout);
	}
#endif

#ifdef ETH_WAKEUP_SUPPORT
	//if((eth & 0x90) == 0x90) {
	if(((eth & 0x10) == 0x10) && (var_wake_param & BIT17)) {		
		//enable Wake On Lan
		if (status & BIT17) {
			DPRINTK(KERN_ALERT "\n[%s]ETH WOL event\n",__func__);
			input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
			input_sync(kpadPower_dev);
			input_report_key(kpadPower_dev, KEY_POWER, 0); //power key is released
			input_sync(kpadPower_dev);
		}
	} else {
		//disable Wake On Lan
		//poweron RGMii
		PMPB_VAL |= 0x00000002;
	}
#endif	

#ifdef CONFIG_BATTERY_WMT
	if(battery_used){
		if (var_wake_param & BIT0){  
			if (status & BIT0) {		
				DPRINTK(KERN_ALERT "\n[%s]Battery low event\n",__func__);
				input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
				input_sync(kpadPower_dev);
				input_report_key(kpadPower_dev, KEY_POWER, 0); //power key is released
				input_sync(kpadPower_dev);
			}
		}
	}
#endif	

	if(var_wake_param & BIT3){//gri
		if (status & BIT3) {		
			DPRINTK(KERN_ALERT "\n[%s]CEC event\n",__func__);
			input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
			input_sync(kpadPower_dev);
			input_report_key(kpadPower_dev, KEY_POWER, 0); //power key is released
			input_sync(kpadPower_dev);
		}
	}
	
#ifdef CIR_WAKEUP_SUPPORT 
	if(var_wake_param & BIT22){
		if (status & BIT22) {		
			DPRINTK(KERN_ALERT "\n[%s]CIR wake up event\n",__func__);
			input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
			input_sync(kpadPower_dev);
			input_report_key(kpadPower_dev, KEY_POWER, 0); //power key is released
			input_sync(kpadPower_dev);
		}
	}
#endif

#endif 
//<-- end removed 


#ifdef RTC_WAKEUP_SUPPORT
	RTAS_VAL = 0x0;       	/* Disable RTC alarm */
#endif

	/*
	 * Force to do once CPR for system.
	 */
	OSM1_VAL = wmt_read_oscr() + LATCH;
	OSTC_VAL |= OSTC_ENABLE;

	//udelay(200);  /* delay for resume not complete */

	//local_irq_enable();  //do it in arch_suspend_enable_irqs() which is called by suspend_enter() in suspend.c
	local_fiq_enable();

	/*
	 * reset wakeup settings
	 */
	//PMWE_VAL |= 0x3; //enable wake-up 0/1

	if (wmt_batt_charge_type() == 1) {
		// is charging???
		if (wmt_read_dcin_state() == 0) {
			pull_charge_elec_pin(0);
		}
		else {
			// charging, set large electricity first. 
			// Then it will auto change to small electricity if usb connectd to pc.
			pull_charge_elec_pin(1);
		}
	}

	return 0;
}


/* wmt_pm_prepare()
 *
 * Called after processes are frozen, but before we shut down devices.
 */
static int wmt_pm_prepare(void)
{
	unsigned int date, time;
	struct timeval tv;

	WMT_WAKE_UP_EVENT = 0;

	/*
	 * Estimate time zone so that wmt_pm_finish can update the GMT time
	 */

	rtc2sys = 0;
    
	if ((*(volatile unsigned int *)SYSTEM_CFG_CTRL_BASE_ADDR)>0x34260102) {        
		wmt_read_rtc(&date, &time);
	}

	do_gettimeofday(&tv);

	rtc2sys = mktime(RTCD_YEAR(date) + ((RTCD_CENT(date) * 100) + 2000),
						RTCD_MON(date),
						RTCD_MDAY(date),
						RTCT_HOUR(time),
						RTCT_MIN(time),
						RTCT_SEC(time));
	rtc2sys = rtc2sys-tv.tv_sec;
	if (rtc2sys > 0)
		rtc2sys += 10;
	else
		rtc2sys -= 10;
	rtc2sys = rtc2sys/60/60;
	rtc2sys = rtc2sys*60*60;

	return 0;
}

void wmt_report_powerkey(void)
{
	spin_lock_irq(&kpadPower_lock);
	printk("[%s]power key pressed\n",__func__);
	input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
	input_sync(kpadPower_dev);
	printk("[%s]power key release\n",__func__);
	input_report_key(kpadPower_dev, KEY_POWER, 0); //power key is released
	input_sync(kpadPower_dev);
	spin_unlock_irq(&kpadPower_lock);

	return;
}


/* wmt_pm_finish()
 *
 * Called after devices are re-setup, but before processes are thawed.
 */
static void wmt_pm_finish(void)
{
	unsigned int date, time;
	struct timespec tv;
	unsigned int status = WMT_WAKE_UP_EVENT;

	char value[32] = {'\0',};
	int len = 31;
#if 0
	struct rtc_time tm;
	unsigned long tmp = 0;
	struct timeval tv1;
#endif
	/* FIXME:
	 * There are a warning when call iounmap here,
	 * please iounmap the mapped virtual ram later */
	// iounmap((void *)exec_at);

	/*
	 * Update kernel time spec.
	 */
	if ((*(volatile unsigned int *)SYSTEM_CFG_CTRL_BASE_ADDR)>0x34260102) {        
		wmt_read_rtc(&date, &time);
	}

	tv.tv_nsec = 0;
	tv.tv_sec = mktime(RTCD_YEAR(date) + ((RTCD_CENT(date) * 100) + 2000),
						RTCD_MON(date),
						RTCD_MDAY(date),
						RTCT_HOUR(time),
						RTCT_MIN(time),
						RTCT_SEC(time));
	/* RTC stores local time, adjust GMT time, tv */
	tv.tv_sec = tv.tv_sec-rtc2sys;
	do_settimeofday(&tv);

//--> added by howayhuo
	if(status == 0)
	    return;

#ifdef KEYPAD_POWER_SUPPORT	
	if (status & BIT14) {
            wmt_report_powerkey();
	}
#endif

#ifdef ETH_WAKEUP_SUPPORT
	//if((eth & 0x90) == 0x90) {
	if(((eth & 0x10) == 0x10) && (var_wake_param & BIT17)) {		
	//enable Wake On Lan
	if (status & BIT17) {
		DPRINTK(KERN_ALERT "\n[%s]ETH WOL event\n",__func__);
		input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
		input_sync(kpadPower_dev);
		input_report_key(kpadPower_dev, KEY_POWER, 0); //power key is released
		input_sync(kpadPower_dev);
	}
	} else {
		//disable Wake On Lan
		//poweron RGMii
		PMPB_VAL |= 0x00000002;
	}
#endif	
			
#ifdef CONFIG_BATTERY_WMT
	if(battery_used){
		if (var_wake_param & BIT0){  
			if (status & BIT0) {		
				DPRINTK(KERN_ALERT "\n[%s]Battery low event\n",__func__);
				input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
				input_sync(kpadPower_dev);
				input_report_key(kpadPower_dev, KEY_POWER, 0); //power key is released
				input_sync(kpadPower_dev);
			}
		}
	}
#endif	
			
	if(var_wake_param & BIT3){//gri
		if (status & BIT3) {		
			DPRINTK(KERN_ALERT "\n[%s]CEC event\n",__func__);
			input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
			input_sync(kpadPower_dev);
			input_report_key(kpadPower_dev, KEY_POWER, 0); //power key is released
			input_sync(kpadPower_dev);
		}
	}

	if (wmt_batt_charge_type() == 1) {
		// wakeup by usb hotplug, light display screen
		if (status & BIT2) {		
			DPRINTK(KERN_ALERT "\n[%s]Wakeup2 event\n",__func__);
			input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
			input_sync(kpadPower_dev);
			input_report_key(kpadPower_dev, KEY_POWER, 0); //power key is released
			input_sync(kpadPower_dev);
		}
	}
				
#ifdef CIR_WAKEUP_SUPPORT 
	if(var_wake_param & BIT22){
		if (status & BIT22) {		
			DPRINTK(KERN_ALERT "\n[%s]CIR wake up event\n",__func__);
			input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
			input_sync(kpadPower_dev);
			input_report_key(kpadPower_dev, KEY_POWER, 0); //power key is released
			input_sync(kpadPower_dev);
		}
	}
#endif
	if (status & BIT1) {
		if (wmt_getsyspara("wmt.modem.wakeuplight", value, &len) == 0 && len > 0) {
			if (value[0] == '1')
				wmt_report_powerkey();
		}
	}

//<-- end add

}

static int wmt_pm_valid(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		return 1;
	default:
		return 0;
	}
}

static struct platform_suspend_ops wmt_pm_ops = {
	.valid			= wmt_pm_valid,
	.prepare        = wmt_pm_prepare,
	.enter          = wmt_pm_enter,
	.finish         = wmt_pm_finish,
};

#if 0
#if defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)

static int procfile_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	len = sprintf(page, "%d\n", softpower_data);
	return len;
}


static int procfile_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char *endp;

	softpower_data = simple_strtoul(buffer, &endp, 0);

	if (softpower_data != 0)
		softpower_data = 1;

/*	printk("%s: %s, softpower_data=[0x%X]\n", DRIVER_NAME, __FUNCTION__, softpower_data );*/
/*	printk("%s: return [%d]\n", DRIVER_NAME, (count + endp - buffer ) );*/

	return (count + endp - buffer);
}

#endif	/* defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)*/
#endif

#ifdef KEYPAD_POWER_SUPPORT
static inline void kpadPower_timeout(unsigned long fcontext)
{

	struct input_dev *dev = (struct input_dev *) fcontext;
	//printk("-------------------------> kpadPower time out\n");
	time2 = jiffies_to_msecs(jiffies);
	if ((time2 - time1) > 2000 && sync_counter > 4) { //2000 msec
		schedule_work(&PMC_sync);
		//DPRINTK("1[%s]dannier count=%d jiffies=%lu, %d\n",__func__, sync_counter, jiffies,jiffies_to_msecs(jiffies));
	} //else
		//DPRINTK("0[%s]dannier count=%d jiffies=%lu, %d\n",__func__, sync_counter, jiffies,jiffies_to_msecs(jiffies));
	DPRINTK(KERN_ALERT "\n[%s]kpadPower time out GPIO_ID_GPIO_VAL = %x\n",__func__,GPIO_ID_GPIO_VAL);
	if(!kpadPower_dev)
		return;

	spin_lock_irq(&kpadPower_lock);

	if(!(PMPB_VAL & BIT24)) {
		input_report_key(dev, KEY_POWER, 0); //power key is released
		input_sync(dev);
		powerKey_is_pressed = 0;
		wmt_pwrbtn_debounce_value(power_on_debounce_value);
		DPRINTK("[%s]power key released\n",__func__);
		sync_counter = 0;
	}else {
		DPRINTK("[%s]power key not released\n",__func__);
		mod_timer(&kpadPower_timer, jiffies + power_button_timeout);
		sync_counter++;
	}

	spin_unlock_irq(&kpadPower_lock);

}
#endif

/*
 * Initialize power management interface
 */
static int __init wmt_pm_init(void)
{
	struct apm_dev_s *pm_dev;
	char buf[256];
	char varname[] = "wmt.pwbn.param";
	int varlen = 256;
	int i;
	char value[32] = {'\0',};
	int len = 31;
	
	//gri
	char wake_buf[80];
	char wake_varname[] = "wmt.pmc.param";
	int wake_varlen = 80;

	char wake_buf_inresume[80];
	char wake_varname_inresume[] = "wmt.pmc.inresume";
	int wake_varlen_inresume = 80;

	char cir_buf[80];
	char cir_varname[] = "wmt.io.rmtctl";
	int cir_varlen = 80;
	
#ifdef ETH_WAKEUP_SUPPORT
	char eth_env_name[] = "wmt.eth.param";
	char eth_env_val[20] = "0";
	int eth_varlen = 5;
#endif

#ifdef CONFIG_BATTERY_WMT
	char bat_varname[] = "wmt.io.bat";
#endif

	if (wmt_getsyspara(cir_varname, cir_buf, &cir_varlen) == 0)
		wmt_cir_enable = 1;

#ifdef KEYPAD_POWER_SUPPORT

	kpadPower_dev = input_allocate_device();
	if(kpadPower_dev) {
		//Initial the static variable
		spin_lock_init(&kpadPower_lock);
		powerKey_is_pressed = 0;
		pressed_jiffies = 0;
		init_timer(&kpadPower_timer);
		kpadPower_timer.function = kpadPower_timeout;
		kpadPower_timer.data = (unsigned long)kpadPower_dev;

		/* Register an input event device. */
		set_bit(EV_KEY,kpadPower_dev->evbit);
		for (i = 0; i < KPAD_POWER_FUNCTION_NUM; i++)
		set_bit(kpadPower_codes[i], kpadPower_dev->keybit);

		kpadPower_dev->name = "kpadPower",
		kpadPower_dev->phys = "kpadPower",


		kpadPower_dev->keycode = kpadPower_codes;
		kpadPower_dev->keycodesize = sizeof(unsigned int);
		kpadPower_dev->keycodemax = KPAD_POWER_FUNCTION_NUM;

		/*
		* For better view of /proc/bus/input/devices
		*/
		kpadPower_dev->id.bustype = 0;
		kpadPower_dev->id.vendor  = 0;
		kpadPower_dev->id.product = 0;
		kpadPower_dev->id.version = 0;
		input_register_device(kpadPower_dev);
	} else
		printk("[wmt_pm_init]Error: No memory for registering Kpad Power\n");
#endif

//--> Initialize CHARGE_ELEC Pin
	memset(&s_charge_elec_pin, 0, sizeof(struct gpio_operation_t));
	if( wmt_getsyspara(ENV_GPIO_CHARGE_ELEC,buf,&varlen) == 0) 
	{
		sscanf(buf,"%d:%d:%x:%x:%x:%x",&s_charge_elec_pin.id, &s_charge_elec_pin.act, &s_charge_elec_pin.bitmap, &s_charge_elec_pin.ctl, \
			&s_charge_elec_pin.oc, &s_charge_elec_pin.od);
		if ((s_charge_elec_pin.ctl&0xffff0000) == GPIO_PHY_BASE_ADDR)
			s_charge_elec_pin.ctl += WMT_MMAP_OFFSET;
		if ((s_charge_elec_pin.oc&0xffff0000) == GPIO_PHY_BASE_ADDR)
			s_charge_elec_pin.oc += WMT_MMAP_OFFSET;
		if ((s_charge_elec_pin.od&0xffff0000) == GPIO_PHY_BASE_ADDR)
			s_charge_elec_pin.od += WMT_MMAP_OFFSET;

		printk("[wmt_pm_init] id = 0x%x, act = 0x%x, bitmap = 0x%x, ctl = 0x%x, oc = 0x%x, od = 0x%x\n", 
			s_charge_elec_pin.id, s_charge_elec_pin.act, s_charge_elec_pin.bitmap, s_charge_elec_pin.ctl,
			s_charge_elec_pin.oc, s_charge_elec_pin.od);
	}
	else
		printk("[%s] Not define CHARGE_ELEC pin\n", __func__);
//<-- end add

//gri
	if (wmt_getsyspara(wake_varname, wake_buf, &wake_varlen) == 0) {
		sscanf(wake_buf,"%x:%x:%x",
						&var_wake_en,
						&var_wake_param,
						&var_wake_type);	
	}
	else {
		var_wake_en = 1;
		var_wake_param = 0x00408001;
		var_wake_type = 0x00002000;
	}


	if (wmt_getsyspara(wake_varname_inresume, wake_buf_inresume, &wake_varlen_inresume) == 0) {
		sscanf(wake_buf_inresume,"%x:%x:%x",
						&var_wake_en_inresume,
						&var_wake_param_inresume,
						&var_wake_type_inresume);	
	}
	PMWT_VAL = var_wake_type_inresume;    
	PMWE_VAL = var_wake_param_inresume;


	
	printk("[%s] var define var_wake_en=%x var_wake_param=%x\n",__func__, var_wake_en, var_wake_param);

#ifdef ETH_WAKEUP_SUPPORT
	if(wmt_getsyspara(eth_env_name,eth_env_val,&eth_varlen) == 0) {
		sscanf(eth_env_val,"%X",&eth);
	}else {
		// not define wmt.eth.param default enable WOL
		/* bit 7 : WOL support ==> remove by griffen for wmt.pmc.param
		 * bit 4 : VIA Velocity driver support
		 * bit 1 : Android framework ETH support
		 */
		//eth = 0x90;
		eth = 0x10;
	}
#endif


#ifdef CONFIG_BATTERY_WMT
	if (wmt_getsyspara(bat_varname, buf, &varlen) == 0) {
		sscanf(buf,"%x", &battery_used);
	}
	printk("[%s] battery_used = %x\n",__func__, battery_used);
#endif

#ifdef CONFIG_CACHE_L2X0
	if(wmt_getsyspara("wmt.l2c.param",buf,&varlen) == 0)
		sscanf(buf,"%d:%x:%x",&l2x0_onoff, &l2x0_aux, &l2x0_prefetch_ctrl );
#endif

	/* Press power button (either hard-power or soft-power) will trigger a power button wakeup interrupt*/
	/* Press reset button will not trigger any PMC wakeup interrupt*/
	/* Hence, force write clear all PMC wakeup interrupts before request PMC wakeup IRQ*/
	PMWS_VAL = PMWS_VAL;

	INIT_WORK(&PMC_shutdown, run_shutdown);
	INIT_WORK(&PMC_sync, run_sync);

#ifdef RTC_WAKEUP_SUPPORT
	PMWE_VAL &= ~(PMWE_RTC);
#endif
	/*
	 *  set interrupt service routine
	 */
/*	if (request_irq(IRQ_PMC_WAKEUP, &pmc_wakeup_isr, SA_SHIRQ, "pmc", &pm_dev) < 0) {*/
	if (request_irq(IRQ_PMC_WAKEUP, &pmc_wakeup_isr,  IRQF_DISABLED, "pmc", &pm_dev) < 0)
		printk(KERN_ALERT "%s: [Wondermedia_pm_init] Failed to register pmc wakeup irq \n"
               , DRIVER_NAME);

#ifndef CONFIG_SKIP_DRIVER_MSG
	/*
	 * Plan to remove it to recude core size in the future.
	 */
	printk(KERN_INFO "%s: WonderMedia Power Management driver\n", DRIVER_NAME);
#endif

	/*
	 * Setup PM core driver into kernel.
	 */
	suspend_set_ops(&wmt_pm_ops);

#if defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)

	/* Power button is configured as soft power*/
	printk("%s: Power button is configured as soft power\n", DRIVER_NAME);
	PMPB_VAL |= PMPB_SOFTPWR;

#if 0
	/* Create proc entry*/
	proc_softpower = create_proc_entry("softpower", 0644, &proc_root);
	proc_softpower->data = &softpower_data;
	proc_softpower->read_proc = procfile_read;
	proc_softpower->write_proc =  procfile_write;
#endif 

#else
	/* Power button is configured as hard power*/
	printk("%s: Power button is configured as hard power\n", DRIVER_NAME);
	PMPB_VAL = 0;

#endif	/* defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)*/

		/*read power button debounce value*/
		if (wmt_getsyspara(varname, buf, &varlen) == 0){
			sscanf(buf,"%d:%d:%d",
						&power_on_debounce_value,
						&resume_debounce_value,
						&power_up_debounce_value);

		if (power_on_debounce_value < min_debounce_value)
			power_on_debounce_value = min_debounce_value;
		if (power_on_debounce_value > max_debounce_value)
			power_on_debounce_value = max_debounce_value;

		if (resume_debounce_value < min_debounce_value)
			resume_debounce_value = min_debounce_value;
		if (resume_debounce_value > max_debounce_value)
			resume_debounce_value = max_debounce_value;

		if (power_up_debounce_value < min_debounce_value)
			power_up_debounce_value = min_debounce_value;
		if (power_up_debounce_value > max_debounce_value)
			power_up_debounce_value = max_debounce_value;
	}


	/*set power button debounce value*/
	printk("[%s] power_on = %d resume = %d power_up = %d\n",
	__func__,power_on_debounce_value, resume_debounce_value, power_up_debounce_value);
	wmt_pwrbtn_debounce_value(power_on_debounce_value);

	return 0;
}

late_initcall(wmt_pm_init);
