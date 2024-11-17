/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "msm_sensor.h"

#define CONFIG_MSMB_CAMERA_DEBUG
#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

#define CAM_DRV_I2C_REG_SMIA          (0x0003)
#define CAM_DRV_I2C_REG_SMIA_MODEL    (0x0000)
#define CAM_DRV_I2C_REG_SMIA_REVISION (0x0002)
#define CAM_DRV_I2C_REG_SMIA_VERSION  (0x0004)
#define CAM_DRV_I2C_REG_ET8EX           (0x04) /* A register in ET8EK8-AS */
#define CAM_DRV_I2C_REG_SMIApp_VERSION  (0x0011)    /*SMIA++ version information register*/
#define CAM_DRV_I2C_REG_SMIApp_MODULE_DATE_PHASE (0x0015)
#define CAMERA_SMIA_SENSOR_REVISION_NUMBER      (0x0018)

/* SMIApp sensor manufacturer ID's */
#define CAM_SMIA65pp_MANUFACTURER_RESERVED    0x00 /*  0 */
#define CAM_SMIA65pp_MANUFACTURER_ST          0x01 /*  1 */
#define CAM_SMIA65pp_MANUFACTURER_AGILENT     0x02 /*  2 */
#define CAM_SMIA65pp_MANUFACTURER_FOVEON      0x03 /*  3 */
#define CAM_SMIA65pp_MANUFACTURER_HYNIX       0x04 /*  4 */
#define CAM_SMIA65pp_MANUFACTURER_IC_MEDIA    0x05 /*  5 */
#define CAM_SMIA65pp_MANUFACTURER_MICRON      0x06 /*  6 */
#define CAM_SMIA65pp_MANUFACTURER_OMNIVISION  0x07 /*  7 */
#define CAM_SMIA65pp_MANUFACTURER_PANASONIC   0x08 /*  8 */
#define CAM_SMIA65pp_MANUFACTURER_SAMSUNG     0x09 /*  9 */
#define CAM_SMIA65pp_MANUFACTURER_SHARP       0x0A /* 10 */
#define CAM_SMIA65pp_MANUFACTURER_SONY        0x0B /* 11 */
#define CAM_SMIA65pp_MANUFACTURER_TOSHIBA     0x0C /* 12 */
#define CAM_SMIA65pp_MANUFACTURER_CYPRESS     0x0D /* 13 */
#define CAM_SMIA65pp_MANUFACTURER_BYD         0x0E /* 14 */
#define CAM_SMIA65pp_MANUFACTURER_FOXCONN     0x0F /* 15 */
#define CAM_SMIA65pp_MANUFACTURER_LITEON      0x11 /* 17 */

/* SMIA65pp sensor model ID's */
#define CAM_SMIA65pp_MODEL_ST_RAISU_VS6955C          0x0B8B /* Used in RAISU 5MP camera module with ST silicon */
#define CAM_SMIA65pp_MODEL_SHARP_DUCATI              0xEACA // Front-facing 5MP Ducati camera made by Sharp
#define CAM_SMIA65pp_MODEL_SHARP_HILL              0x2140 // Supposedly the back camera
struct smia65pp_sensor_module_info {
    uint8_t manufacturer_id;
    uint16_t module_id;
    uint8_t revision;
    const char      *name;
};
const struct smia65pp_sensor_module_info smia65pp_sensor_module_table[] = {
    {CAM_SMIA65pp_MANUFACTURER_ST,CAM_SMIA65pp_MODEL_ST_RAISU_VS6955C,1,"smia65pp010b8b01"},
    {CAM_SMIA65pp_MANUFACTURER_SHARP, CAM_SMIA65pp_MODEL_SHARP_DUCATI, 5, "Ducati 5MP Front-Facing Camera"},
    {CAM_SMIA65pp_MANUFACTURER_SHARP, CAM_SMIA65pp_MODEL_SHARP_HILL, 0, "Faker Sony Camera idk"},
};



#define SMIA65PP_SENSOR_NAME "smia65pp"
DEFINE_MSM_MUTEX(smia65pp_mut);

static struct msm_sensor_ctrl_t smia65pp_s_ctrl;

static struct msm_sensor_power_setting smia65pp_power_setting[] = {
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VANA,
		.config_val = 0,
		.delay = 1,
	},
    {
        .seq_type = SENSOR_VREG,
        .seq_val = CAM_VDIG,
        .config_val = 0,
        .delay = 1,
    },
	{
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_RESET,
        .config_val = GPIO_OUT_LOW,
        .delay = 1,
    },
    {
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_RESET,
        .config_val = GPIO_OUT_HIGH,
        .delay = 30,
    },
    {
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 9600000,
		.delay = 30,//15,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 1,
	},
};

static struct v4l2_subdev_info smia65pp_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SGRBG10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};

static const struct i2c_device_id smia65pp_i2c_id[] = {
	{SMIA65PP_SENSOR_NAME, (kernel_ulong_t)&smia65pp_s_ctrl},
	{ }
};

static int32_t msm_smia65pp_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &smia65pp_s_ctrl);
}

static struct i2c_driver smia65pp_i2c_driver = {
	.id_table = smia65pp_i2c_id,
	.probe  = msm_smia65pp_i2c_probe,
	.driver = {
		.name = SMIA65PP_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client smia65pp_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id smia65pp_dt_match[] = {
	{.compatible = "qcom,smia65pp", .data = &smia65pp_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, smia65pp_dt_match);

static struct platform_driver smia65pp_platform_driver = {
	.driver = {
		.name = "qcom,smia65pp",
		.owner = THIS_MODULE,
		.of_match_table = smia65pp_dt_match,
	},
};

static int32_t smia65pp_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
		
	match = of_match_device(smia65pp_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
		
	return rc;
}

static int __init smia65pp_init_module(void)
{
	int32_t rc = 0;
	pr_info("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&smia65pp_platform_driver,
		smia65pp_platform_probe);
	if (!rc)
		return rc;
	pr_err("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&smia65pp_i2c_driver);
}

static void __exit smia65pp_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	if (smia65pp_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&smia65pp_s_ctrl);
		platform_driver_unregister(&smia65pp_platform_driver);
	} else
		i2c_del_driver(&smia65pp_i2c_driver);
	return;
}

int32_t smia65pp_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    uint16_t chipid = 0;
    uint16_t manufacturer_id = 0;
    uint16_t revision = 0;
    uint16_t module_count = sizeof(smia65pp_sensor_module_table)/sizeof(smia65pp_sensor_module_table[0]);
    uint16_t i= 0;

    rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
            s_ctrl->sensor_i2c_client,
            CAM_DRV_I2C_REG_SMIA_MODEL,
            &chipid, MSM_CAMERA_I2C_WORD_DATA);
					
    if (rc < 0) {
        pr_err("%s: %s: read chipid failed\n", __func__,
            s_ctrl->sensordata->sensor_name);
        return rc;
    }
    rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
        s_ctrl->sensor_i2c_client,
        CAM_DRV_I2C_REG_SMIA,
        &manufacturer_id, MSM_CAMERA_I2C_BYTE_DATA);
				
    if (rc < 0) {
        pr_err("%s: %s: read manufacturer_id failed\n", __func__,
            s_ctrl->sensordata->sensor_name);
        return rc;
    }
    rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
    s_ctrl->sensor_i2c_client,
    CAM_DRV_I2C_REG_SMIA_REVISION,
    &revision, MSM_CAMERA_I2C_BYTE_DATA);
			
    if (rc < 0) {
        pr_err("%s: %s: read revision failed\n", __func__,
            s_ctrl->sensordata->sensor_name);
        return rc;
    }
    for(i= 0; i<module_count; i++)
    {
        CDBG("%s: read id:%02x,%04x,%02x, expected id %02x,%04x,%02x,\n", __func__,manufacturer_id, chipid,revision,
        smia65pp_sensor_module_table[i].manufacturer_id,smia65pp_sensor_module_table[i].module_id,smia65pp_sensor_module_table[i].revision);

      if (chipid == smia65pp_sensor_module_table[i].module_id
            && manufacturer_id == smia65pp_sensor_module_table[i].manufacturer_id
            && revision == smia65pp_sensor_module_table[i].revision
            ) {
            s_ctrl->sensordata->sensor_name = smia65pp_sensor_module_table[i].name;
            CDBG("smia65pp_match_id: %s,id match successed!\n",s_ctrl->sensordata->sensor_name);
            break;
        }
    }
    if (i == module_count) {
        pr_err("smia65pp_match_id chip id does not match\n");
        return -ENODEV;
    }    
    return rc;
}



static struct msm_sensor_fn_t smia65pp_sensor_func_tbl = {
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = smia65pp_match_id,
};



static struct msm_sensor_ctrl_t smia65pp_s_ctrl = {
	.sensor_i2c_client = &smia65pp_sensor_i2c_client,
	.power_setting_array.power_setting = smia65pp_power_setting,
	.power_setting_array.size = ARRAY_SIZE(smia65pp_power_setting),
	.msm_sensor_mutex = &smia65pp_mut,
	.sensor_v4l2_subdev_info = smia65pp_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(smia65pp_subdev_info),
	.func_tbl = &smia65pp_sensor_func_tbl,
};

module_init(smia65pp_init_module);
module_exit(smia65pp_exit_module);
MODULE_DESCRIPTION("smia65pp");
MODULE_LICENSE("GPL v2");