/*
 * ADC driver for Maxim MAX8907c
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/mfd/max8907c.h>
#include <linux/mfd/max8907_reg.h>
#include <linux/mfd/max8907_adc.h>
#include <linux/delay.h>

struct max8907c_adc_info {
	struct i2c_client	*i2c_power;
	struct i2c_client	*i2c_adc;
	struct max8907c		*chip;
};

static struct i2c_client *max8907c_i2c_adc_client = NULL;
static struct i2c_client *max8907c_i2c_power_client = NULL;
struct mutex adc_en_lock;
struct mutex adc_en_temp_lock;


/**
 * max8907c_adc_read_aux2 - Return ADC value for aux2
 *
 * Will return the ADC value of AUX2 port of max8907c.
 * ADC function converts 0.0V ~ 2.5V to 12bit digital value, 
 * which ranges from 0 ~ 4095(0xfff).
 *
 *  (return value) = (ADC read) * 2500 / 0xfff   
 *
 * range 0 ~ 2500 mV
 */
int max8907c_adc_read_aux2(int *mili_volt)
{
	u32 tmp;
	int msb, lsb;
	int ret;

	if (WARN(mili_volt == NULL, "%s() mili_volt is null\n", __func__))
		return -ENXIO;

	*mili_volt = 0;

	mutex_lock(&adc_en_lock);

	/* Enable ADCREF by setting the INT_REF_EN bit in the RESET_CNFG register */
	ret = max8907c_set_bits(max8907c_i2c_power_client, MAX8907C_REG_RESET_CNFG, 0x01, 0x01);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907C_REG_RESET_CNFG, ret);
		goto error;
	}

	/* Enable internal voltage reference. 
	 * Write 0x12 to MAX8907_TSC_CNFG1 to turn on internal reference. */
	ret = max8907c_reg_write(max8907c_i2c_adc_client, MAX8907_TSC_CNFG1, 0x12);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907_TSC_CNFG1, ret);
		goto error;
	}

	/* Send MAX8907_ADC_CMD_AUX2M_OFF command to powerup and the ADC and perform a conversion. */
	ret = max8907c_send_cmd(max8907c_i2c_adc_client, MAX8907_ADC_CMD_AUX2M_OFF);

	if (ret < 0) {
		pr_err("%s() failed send command %x returned %d\n", __func__, MAX8907_ADC_CMD_AUX2M_OFF, ret);
		goto error;
	}

#ifndef	CONFIG_MACH_STAR
	/* Turn off the internal reference. Write 0x11 to register MAX8907_TSC_CNFG1 */
	ret = max8907c_reg_write(max8907c_i2c_adc_client, MAX8907_TSC_CNFG1, 0x11);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907_TSC_CNFG1, ret);
		goto error;
	}
	udelay(300);
#endif

	/* Read ADC result. */
	msb = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_ADC_RES_AUX2_MSB);
	lsb = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_ADC_RES_AUX2_LSB);

#ifndef	COFNIG_MACH_STAR
	/* Disable ADCREF by setting the INT_REF_EN bit 0. in the RESET_CNFG register */
	ret = max8907c_set_bits(max8907c_i2c_power_client, MAX8907C_REG_RESET_CNFG, 0x01, 0x00);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907C_REG_RESET_CNFG, ret);
		goto error;
	}
#endif

	mutex_unlock(&adc_en_lock);

	/* Convert the result value to mV. */
	tmp = ((msb << 4)|(lsb >> 4)) * 2500;
	*mili_volt = tmp / 0xfff;

	return 0;

error:
	mutex_unlock(&adc_en_lock);
	return ret;

}

int max8907c_adc_read_volt(int *mili_volt)
{
	u32 tmp;
	int msb, lsb;
	int ret;

	if (WARN(mili_volt == NULL, "%s() mili_volt is null\n", __func__))
		return -ENXIO;

	*mili_volt = 0;

	mutex_lock(&adc_en_lock);

	/* Enable ADCREF by setting the INT_REF_EN bit in the RESET_CNFG register */
	ret = max8907c_set_bits(max8907c_i2c_power_client, MAX8907C_REG_RESET_CNFG, 0x01, 0x01);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907C_REG_RESET_CNFG, ret);
		goto error;
	}

	/* Enable internal voltage reference. 
	 * Write 0x12 to MAX8907_TSC_CNFG1 to turn on internal reference. */
	ret = max8907c_reg_write(max8907c_i2c_adc_client, MAX8907_TSC_CNFG1, 0x12);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907_TSC_CNFG1, ret);
		goto error;
	}

	/* Send MAX8907_ADC_CMD_AUX2M_OFF command to powerup and the ADC and perform a conversion. */
#ifdef	CONFIG_MACH_STAR
	ret = max8907c_reg_write(max8907c_i2c_adc_client, CONV_REG_VMBATT_ON, 0x00);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, CONV_REG_VMBATT_ON, ret);
		goto error;
	}
#else
	ret = max8907c_send_cmd(max8907c_i2c_adc_client, CONV_REG_VMBATT_ON);
	if (ret < 0) {
		pr_err("%s() failed send command %x returned %d\n", __func__, CONV_REG_VMBATT_ON, ret);
		goto error;
	}
#endif

#ifndef	CONFIG_MACH_STAR
	/* Turn off the internal reference. Write 0x11 to register MAX8907_TSC_CNFG1 */
	ret = max8907c_reg_write(max8907c_i2c_adc_client, MAX8907_TSC_CNFG1, 0x11);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907_TSC_CNFG1, ret);
		goto error;
	}
	udelay(300);
#endif

	/* Read ADC result. */
	msb = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_VMBATT_MSB);
	lsb = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_VMBATT_LSB);

#ifndef	CONFIG_MACH_STAR
	/* Disable ADCREF by setting the INT_REF_EN bit 0. in the RESET_CNFG register */
	ret = max8907c_set_bits(max8907c_i2c_power_client, MAX8907C_REG_RESET_CNFG, 0x01, 0x00);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907C_REG_RESET_CNFG, ret);
		goto error;
	}
#endif
	mutex_unlock(&adc_en_lock);

	/* Convert the result value to mV. */
	tmp = ((msb << 4)|(lsb >> 4));
	*mili_volt = tmp * 2;

	return 0;
error:
	mutex_unlock(&adc_en_lock);
	return ret;

}
EXPORT_SYMBOL(max8907c_adc_read_volt);

int max8907c_adc_read_temp(unsigned int *mili_temp)
{
	u32 tmp;
	unsigned char msb, lsb;
	int ret;

	if (WARN(mili_temp == NULL, "%s() mili_temp is null\n", __func__))
		return -ENXIO;

	*mili_temp = 0;

	mutex_lock(&adc_en_temp_lock);

	/* Enable ADCREF by setting the INT_REF_EN bit in the RESET_CNFG register */
	ret = max8907c_set_bits(max8907c_i2c_power_client, MAX8907C_REG_RESET_CNFG, 0x01, 0x01);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907C_REG_RESET_CNFG, ret);
		goto error;
	}

	/* Enable internal voltage reference. 
	 * Write 0x12 to MAX8907_TSC_CNFG1 to turn on internal reference. */
	ret = max8907c_reg_write(max8907c_i2c_adc_client, MAX8907_TSC_CNFG1, 0x12);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907_TSC_CNFG1, ret);
		goto error;
	}

	/* Send MAX8907_ADC_CMD_AUX2M_OFF command to powerup and the ADC and perform a conversion. */
#ifdef	CONFIG_MACH_STAR
	ret = max8907c_reg_write(max8907c_i2c_adc_client, CONV_REG_THM_ON, 0x00);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, CONV_REG_THM_ON, ret);
		goto error;
	}
#else
	ret = max8907c_send_cmd(max8907c_i2c_adc_client, CONV_REG_THM_ON);
	if (ret < 0) {
		pr_err("%s() failed send command %x returned %d\n", __func__, MAX8907_ADC_CMD_AUX2M_OFF, ret);
		goto error;
	}
#endif

#ifndef	CONFIG_MACH_STAR
	/* Turn off the internal reference. Write 0x11 to register MAX8907_TSC_CNFG1 */
	ret = max8907c_reg_write(max8907c_i2c_adc_client, MAX8907_TSC_CNFG1, 0x11);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907_TSC_CNFG1, ret);
		goto error;
	}
	udelay(300);
#endif

	/* Read ADC result. */
	msb = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_THM_MSB);
	lsb = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_THM_LSB);

#ifndef	CONFIG_MACH_STAR
	/* Disable ADCREF by setting the INT_REF_EN bit 0. in the RESET_CNFG register */
	ret = max8907c_set_bits(max8907c_i2c_power_client, MAX8907C_REG_RESET_CNFG, 0x01, 0x00);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907C_REG_RESET_CNFG, ret);
		goto error;
	}
#endif
	mutex_unlock(&adc_en_temp_lock);

	tmp = msb;
	tmp = ((tmp << 4) | (lsb >> 4));
	*mili_temp = tmp;

	return 0;
error:
	mutex_unlock(&adc_en_temp_lock);
	return ret;
}
EXPORT_SYMBOL(max8907c_adc_read_temp);


u32 max8907c_adc_read_hook_adc(void)
{
	u32 tmp;
	unsigned char msb, lsb;
	int ret;

	mutex_lock(&adc_en_temp_lock);

	/* Enable ADCREF by setting the INT_REF_EN bit in the RESET_CNFG register */
	ret = max8907c_set_bits(max8907c_i2c_power_client, MAX8907C_REG_RESET_CNFG, 0x01, 0x01);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907C_REG_RESET_CNFG, ret);
		goto error;
	}

	/* Enable internal voltage reference. 
	 * Write 0x12 to MAX8907_TSC_CNFG1 to turn on internal reference. */
	ret = max8907c_reg_write(max8907c_i2c_adc_client, MAX8907_TSC_CNFG1, 0x12);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907_TSC_CNFG1, ret);
		goto error;
	}

	/* Send MAX8907_ADC_CMD_AUX2M_OFF command to powerup and the ADC and perform a conversion. */
	ret = max8907c_reg_write(max8907c_i2c_adc_client, CONV_REG_AUX2_ON, 0x00); // R:8F, W:8E
	if (ret < 0) {
		pr_err("%s() failed send command %x returned %d\n", __func__, MAX8907_ADC_CMD_AUX2M_OFF, ret);
		goto error;
	}

#ifndef	CONFIG_MACH_STAR
	/* Turn off the internal reference. Write 0x11 to register MAX8907_TSC_CNFG1 */
	ret = max8907c_reg_write(max8907c_i2c_adc_client, MAX8907_TSC_CNFG1, 0x11);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907_TSC_CNFG1, ret);
		goto error;
	}
	udelay(300);
#endif

	/* Read ADC result. */
	msb = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_AUX2_MSB);
	lsb = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_AUX2_LSB);

#ifndef	CONFIG_MACH_STAR
	/* Disable ADCREF by setting the INT_REF_EN bit 0. in the RESET_CNFG register */
	ret = max8907c_set_bits(max8907c_i2c_power_client, MAX8907C_REG_RESET_CNFG, 0x01, 0x00);
	if (ret < 0) {
		pr_err("%s() failed writing on register %x returned %d\n", __func__, MAX8907C_REG_RESET_CNFG, ret);
		goto error;
	}
#endif

	mutex_unlock(&adc_en_temp_lock);

	/* Convert the result value to mV. */
	tmp = ((msb << 4)|(lsb >> 4));

	return tmp;

error:
	mutex_unlock(&adc_en_temp_lock);
	return ret;

}
EXPORT_SYMBOL(max8907c_adc_read_hook_adc);

unsigned int
Max8907BatteryTemperature(
		unsigned int VBatSense,
		unsigned int VBatTemp)
{
	static short BAT_TEMP_TABLE[] = BAT_T_TABLE;
	pr_debug("BatTemp[K] = %d(%d[C]) \n", BAT_TEMP_TABLE[VBatTemp], (BAT_TEMP_TABLE[VBatTemp]-2730));
	return BAT_TEMP_TABLE[VBatTemp];
}
EXPORT_SYMBOL(Max8907BatteryTemperature);

// Interrupts
#define MAX8907_CHG_STAT_VCHG_OK_SHIFT     0x7
#define MAX8907_CHG_STAT_VCHG_OK_MASK      0x1

unsigned char Max8907BatteryChargerOK(unsigned char *status)
{
	unsigned char data = 0;
	data = max8907c_reg_read(max8907c_i2c_adc_client, (MAX8907C_REG_CHG_STAT & 0xFF));

	data = (data >> MAX8907_CHG_STAT_VCHG_OK_SHIFT) & MAX8907_CHG_STAT_VCHG_OK_MASK;
	*status = (data == 0 ? 0 : 1 ); 
	return 1;
}

unsigned char
Max8907GetAcLineStatus(
		unsigned int hDevice,
		unsigned int *pStatus)
{
	unsigned char acLineStatus = 0;
	if (!Max8907BatteryChargerOK(&acLineStatus))
	{
		pr_err("[NVODM PMU] Max8907GetAcLineStatus: Error in checking main charger presence.\n");
		return 0;
	}

	if (acLineStatus == 1)
		*pStatus = 1;
	else
		*pStatus = 0;
	return 1;
}
EXPORT_SYMBOL(Max8907GetAcLineStatus);

#if defined(CONFIG_MACH_STAR)
int max8907c_adc_battery_read_volt(int *mili_volt)
{
	int voltage;


	unsigned char	data, msb_data, lsb_data;
	unsigned int	batvdata;

	if (WARN(mili_volt == NULL, "%s() mili_volt is null\n", __func__))
		return -ENXIO;

	*mili_volt = 0;

	mutex_lock(&adc_en_lock);

	// Enable int_ref_en bit in RESET_CNFG Reg
	data = max8907c_reg_read(max8907c_i2c_power_client, MAX8907_RESET_CNFG);

	data |= (MAX8907_RESET_CNFG_INT_REF_EN_MASK <<
			MAX8907_RESET_CNFG_INT_REF_EN_SHIFT);

	if (max8907c_reg_write(max8907c_i2c_power_client, MAX8907_RESET_CNFG, data) < 0)
	{
		mutex_unlock(&adc_en_lock);
		return 0;
	}

	// Enable Internal voltage reference
	if (max8907c_reg_write(max8907c_i2c_adc_client, MAX8907_TSC_CNFG1, 0x12) < 0)
	{
		mutex_unlock(&adc_en_lock);
		return 0;
	}

	// Send command to powerup and the ADC perform a conversion
	if (max8907c_reg_write(max8907c_i2c_adc_client, CONV_REG_VMBATT_ON, 0x00) < 0)
	{
		mutex_unlock(&adc_en_lock);
		return 0;
	}

	// Get result
	msb_data = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_VMBATT_MSB);
	lsb_data = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_VMBATT_LSB);

	mutex_unlock(&adc_en_lock);

	batvdata = (msb_data << 4) | (lsb_data >> 4) ;
	*mili_volt = batvdata*2; //This conversion is for 12bit ADC result : 8.192*CODE/2^N = 8.192*CODE/4096 = CODE*2 [mV]

	return 0;
}
EXPORT_SYMBOL(max8907c_adc_battery_read_volt);

int max8907c_adc_battery_read_temp(unsigned int *mili_temp)
{

	unsigned char	data, msb_data, lsb_data;
	unsigned int	btempdata, btemp;

	if (WARN(mili_temp == NULL, "%s() mili_temp is null\n", __func__))
		return -ENXIO;

	*mili_temp = 0;	

	mutex_lock(&adc_en_temp_lock);

	// Enable int_ref_en bit in RESET_CNFG Reg
	data = max8907c_reg_read(max8907c_i2c_power_client, MAX8907_RESET_CNFG);

	data |= (MAX8907_RESET_CNFG_INT_REF_EN_MASK <<
			MAX8907_RESET_CNFG_INT_REF_EN_SHIFT);

	if (max8907c_reg_write(max8907c_i2c_power_client, MAX8907_RESET_CNFG, data) < 0)
	{
		mutex_unlock(&adc_en_lock);
		return 0;
	}

	// Enable Internal voltage reference
	if (max8907c_reg_write(max8907c_i2c_adc_client, MAX8907_TSC_CNFG1, 0x12) < 0)
	{
		mutex_unlock(&adc_en_lock);
		return 0;
	}

	// Send command to powerup and the ADC perform a conversion
	if (max8907c_reg_write(max8907c_i2c_adc_client, CONV_REG_THM_ON, 0x00) < 0)
	{
		mutex_unlock(&adc_en_lock);
		return 0;
	}

	// Get result
	msb_data = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_THM_MSB);
	lsb_data = max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_THM_LSB);

	mutex_unlock(&adc_en_temp_lock);

	btemp = btempdata = (msb_data << 4) | (lsb_data >> 4) ;

	*mili_temp = btemp;
	return 0;
}
EXPORT_SYMBOL(max8907c_adc_battery_read_temp);
#endif

static int __devinit max8907c_adc_probe(struct platform_device *pdev)
{
	struct max8907c *chip = dev_get_drvdata(pdev->dev.parent);
	struct max8907c_adc_info *info;

	pr_info("max8907c_adc_probe\n");

	info = kzalloc(sizeof(struct max8907c_adc_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	info->i2c_adc = chip->i2c_adc;
	info->i2c_power = chip->i2c_power;
	info->chip = chip;

	dev_set_drvdata(&pdev->dev, info);

	platform_set_drvdata(pdev, info);
	max8907c_i2c_adc_client = info->i2c_adc;
	max8907c_i2c_power_client = info->i2c_power;

	mutex_init(&adc_en_lock);
	if (!max8907c_i2c_adc_client){
		pr_err("max8907c_adc_probe adc_en_lock\n");
		return -EINVAL;
	}

	mutex_init(&adc_en_temp_lock);
	if (!max8907c_i2c_adc_client){
		pr_err("max8907c_adc_probe adc_en_temp_lock\n");
		return -EINVAL;
	}

	/* default readings. */
	max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_ADC_RES_CNFG1);
	max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_ADC_AVG_CNFG1);
	max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_ADC_ACQ_CNFG1);
	max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_ADC_ACQ_CNFG2);
	max8907c_reg_read(max8907c_i2c_adc_client, MAX8907_ADC_SCHED);
	/* Set ADC reading environment. */
	max8907c_set_bits(max8907c_i2c_adc_client, MAX8907_ADC_RES_CNFG1, 0x40, 0x00);
	max8907c_set_bits(max8907c_i2c_adc_client, MAX8907_ADC_AVG_CNFG1, 0x40, 0x40);
	max8907c_set_bits(max8907c_i2c_adc_client, MAX8907_ADC_ACQ_CNFG1, 0x20, 0x20);
	max8907c_set_bits(max8907c_i2c_adc_client, MAX8907_ADC_SCHED, 0x03, 0x01);

	pr_info("max8907c_adc_probe ok\n");
	return 0;
}

static int __devexit max8907c_adc_remove(struct platform_device *pdev)
{
	struct max8907c_adc_info *info = platform_get_drvdata(pdev);

	if (info)
		kfree(info);
	return 0;
}

static struct platform_driver max8907c_adc_driver = {
	.driver		= {
		.name	= "max8907c-adc",
		.owner	= THIS_MODULE,
	},
	.probe		= max8907c_adc_probe,
	.remove		= __devexit_p(max8907c_adc_remove),
};

static int __init max8907c_adc_init(void)
{
	return platform_driver_register(&max8907c_adc_driver);

}
subsys_initcall(max8907c_adc_init);

static void __exit max8907c_adc_exit(void)
{
	platform_driver_unregister(&max8907c_adc_driver);
}
module_exit(max8907c_adc_exit);

MODULE_DESCRIPTION("Maxim MAX8907C ADC driver");
MODULE_LICENSE("GPL");
