/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include "msm_sd.h"
#include "msm_cci.h"
#include "msm_eeprom.h"

//#define MSM_EEPROM_DEBUG

#undef CDBG
#ifdef MSM_EEPROM_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#endif

DEFINE_MSM_MUTEX(msm_eeprom_mutex);


static int eeprom_config_read_cal_data(struct msm_eeprom_ctrl_t *e_ctrl,
	struct msm_eeprom_cfg_data *cdata)
{
	int rc;

	/* check range */
	if (cdata->cfg.read_data.num_bytes >
		e_ctrl->cal_data.num_data) {
		CDBG("%s: Invalid size. exp %u, req %u\n", __func__,
			e_ctrl->cal_data.num_data,
			cdata->cfg.read_data.num_bytes);
		return -EINVAL;
	}
	if (!e_ctrl->cal_data.mapdata)
		return -EFAULT;
	rc = copy_to_user(cdata->cfg.read_data.dbuffer,
		e_ctrl->cal_data.mapdata,
		cdata->cfg.read_data.num_bytes);

	return rc;
}

static int32_t msm_eeprom_config(struct msm_eeprom_ctrl_t *e_ctrl,
	void __user *argp)
{
	struct msm_eeprom_cfg_data *cdata =
		(struct msm_eeprom_cfg_data *)argp;
	int32_t rc = 0;

	CDBG("%s E\n", __func__);
	switch (cdata->cfgtype) {
	case CFG_EEPROM_GET_INFO:
		CDBG("%s E CFG_EEPROM_GET_INFO\n", __func__);
		cdata->is_supported = e_ctrl->is_supported;
		memcpy(cdata->cfg.eeprom_name,
			e_ctrl->eboard_info->eeprom_name,
			sizeof(cdata->cfg.eeprom_name));
		break;
	case CFG_EEPROM_GET_CAL_DATA:
		CDBG("%s E CFG_EEPROM_GET_CAL_DATA\n", __func__);
		cdata->cfg.get_data.num_bytes =
			e_ctrl->cal_data.num_data;
		break;
	case CFG_EEPROM_READ_CAL_DATA:
		CDBG("%s E CFG_EEPROM_READ_CAL_DATA\n", __func__);
		rc = eeprom_config_read_cal_data(e_ctrl, cdata);
		break;
	default:
		break;
	}

	CDBG("%s X\n", __func__);
	return rc;
}
static int32_t msm_eeprom_get_subdev_id(
	struct msm_eeprom_ctrl_t *e_ctrl, void *arg)
{
	uint32_t *subdev_id = (uint32_t *)arg;
	CDBG("%s E\n", __func__);
	if (!subdev_id) {
		pr_err("%s failed\n", __func__);
		return -EINVAL;
	}
	*subdev_id = e_ctrl->subdev_id;
	CDBG("subdev_id %u\n", *subdev_id);
	CDBG("%s X\n", __func__);
	return 0;
}

static long msm_eeprom_subdev_ioctl(struct v4l2_subdev *sd,
		unsigned int cmd, void *arg)
{
	struct msm_eeprom_ctrl_t *e_ctrl = v4l2_get_subdevdata(sd);
	void __user *argp = (void __user *)arg;

	CDBG("%s:%d a_ctrl %p argp %p\n", __func__, __LINE__, e_ctrl, argp);
	switch (cmd) {
	case VIDIOC_MSM_SENSOR_GET_SUBDEV_ID:
		return msm_eeprom_get_subdev_id(e_ctrl, argp);
	case VIDIOC_MSM_EEPROM_CFG:
		return msm_eeprom_config(e_ctrl, argp);
	default:
		return -ENOIOCTLCMD;
	}

}

static struct msm_camera_i2c_fn_t msm_eeprom_qup_func_tbl = {
	.i2c_read = msm_camera_qup_i2c_read,
	.i2c_read_seq = msm_camera_qup_i2c_read_seq,
	.i2c_write = msm_camera_qup_i2c_write,
	.i2c_write_table = msm_camera_qup_i2c_write_table,
	.i2c_write_seq_table = msm_camera_qup_i2c_write_seq_table,
	.i2c_write_table_w_microdelay =
	msm_camera_qup_i2c_write_table_w_microdelay,
};

static int msm_eeprom_open(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh) {
	struct msm_eeprom_ctrl_t *e_ctrl =  v4l2_get_subdevdata(sd);
	CDBG("%s E\n", __func__);
	if (!e_ctrl) {
		pr_err("%s failed e_ctrl is NULL\n", __func__);
		return -EINVAL;
	}
	CDBG("%s X\n", __func__);
	return 0;
}

static int msm_eeprom_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh) {

	struct msm_eeprom_ctrl_t *e_ctrl =  v4l2_get_subdevdata(sd);
	CDBG("%s E\n", __func__);
	if (!e_ctrl) {
		pr_err("%s failed e_ctrl is NULL\n", __func__);
		return -EINVAL;
	}
	CDBG("%s X\n", __func__);
	return 0;
}

static const struct v4l2_subdev_internal_ops msm_eeprom_internal_ops = {
	.open = msm_eeprom_open,
	.close = msm_eeprom_close,
};

/* Data Upload/Download Interface capability register */
#define CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF_CAPABILITY 0x1800

/* Data Upload/Download Interface set-up register */
#define CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_CTRL               0x0A00
#define CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_STATUS             0x0A01
#define CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_PAGE_SELECT        0x0A02
#define CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_DATA_0             0x0A04
#define CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_DATA_63            0x0A43
#define CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF2_CTRL               0x0A44
#define CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF2_STATUS             0x0A45
#define CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF2_PAGE_SELECT        0x0A46
#define CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF2_DATA_0             0x0A48
#define CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF2_DATA_63            0x0A87


/* NVM related constants */
#define CAM_SMIApp_NVM_BYTES_PER_PAGE           64
#define CAM_SMIApp_NVM_CONFIG_1_REG_ADDR        0x01
#define CAM_SMIApp_NVM_MEM_ADDR_REG_ADDR        (CAM_SMIApp_NVM_CONFIG_1_REG_ADDR + 1)


/** Structure that defines Data upload/download interface details of the sensor */
typedef struct {
	unsigned int if_1_supported                 :1; /**< Indicates whether interface 1 is supported */
	unsigned int if_2_supported                 :1; /**< Indicates whether interface 2 is supported */
	unsigned int polling_needed_in_reading      :1; /**< Indicates whether polling is needed while reading - preapration time for the interface for reading */
	unsigned int polling_needed_in_writing      :1; /**< Indicates whether polling is needed while reading - preapration time for the interface for writing */
} CAM_DRV_SMIApp_DATA_UPLOAD_DOWNLOAD_IF_T;

static uint32_t nvm_calculate_checsum(uint8_t* pBuf, uint32_t endAddr)
{
	uint8_t c0=0;
	uint8_t c1=0;
	uint32_t c0_temp=0;
	uint32_t c1_temp=0;

	uint32_t i;
	for(i=0; i<endAddr; i++) {
		c0_temp=c0+pBuf[i];
		c0=(c0_temp&0xFF)+((c0_temp&0x100)>>8);
		c1_temp=c1+c0;
		c1=(c1_temp&0xFF)+((c1_temp&0x100)>>8);
	}
	return ((c1<<8)+c0);
}

static int32_t smiapp_eeprom_read_data_interface(struct msm_eeprom_ctrl_t *e_ctrl,CAM_DRV_SMIApp_DATA_UPLOAD_DOWNLOAD_IF_T *p_data_upload_download_interface)
{
	int rc = 0;
	uint16_t dummy = 0;
	rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_read(
		&(e_ctrl->i2c_client), CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF_CAPABILITY,
		&dummy,MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s: read failed ,rc:%d\n", __func__ ,rc);
		return rc;
	}
	p_data_upload_download_interface->if_1_supported = dummy & 0x1;
	p_data_upload_download_interface->if_2_supported = (dummy >> 1) & 0x1;
	p_data_upload_download_interface->polling_needed_in_reading = (dummy >> 2) & 0x1;
	p_data_upload_download_interface->polling_needed_in_writing = (dummy >> 3) & 0x1;
	CDBG("dummy:%02x,supported :if_1 %d, if_2 %d,polling_in_reading %d,polling_in_writing %d \n",dummy,p_data_upload_download_interface->if_1_supported,p_data_upload_download_interface->if_2_supported,
		p_data_upload_download_interface->polling_needed_in_reading,p_data_upload_download_interface->polling_needed_in_writing);
	return rc;
}

static int32_t smiapp_eeprom_select_page(struct msm_eeprom_ctrl_t *e_ctrl,uint16_t page_no)
{
	int rc = 0;
	rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_write(
	&(e_ctrl->i2c_client), CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_PAGE_SELECT,
	page_no,MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s: smiapp_eeprom_select_page failed\n", __func__);
	}
	return rc;
}

static int32_t smiapp_eeprom_interface_enable(struct msm_eeprom_ctrl_t *e_ctrl,uint8_t enable)
{
	int rc = 0;
	uint8_t reg_value= 0;
	/* Set up control register - bit 0 - 1: Interface enable / 0: Interface disable ; bit 1 - 0: Read enable / 1: Write enable(bit 1).
	This needs to be enabled only once, hence for page 0 */
	if(enable) {
		reg_value = 1;
	} else {
		/*disable and clear error status */
		reg_value = 4;
	}

	rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_write(
		&(e_ctrl->i2c_client), CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_CTRL,
		reg_value,MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s: smiapp_eeprom_interface_enable failed\n", __func__);
	}
	return rc;
}
static int32_t smiapp_eeprom_waiting_data_ready(struct msm_eeprom_ctrl_t *e_ctrl,uint32_t polling_needed_in_reading)
{
	int rc = 0;
	uint16_t status_reg_val;
	uint16_t max_trials_for_polling = 50; /* Each trial will be after 1ms. So will corresponds to the max wait time in ms, say 50ms */
	rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_read(
		&(e_ctrl->i2c_client), CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_STATUS,
		&status_reg_val,MSM_CAMERA_I2C_BYTE_DATA);



	if (polling_needed_in_reading) {
		for (max_trials_for_polling = 50; !(status_reg_val & 0x1) && max_trials_for_polling; max_trials_for_polling--) {
			/* Poll for read interface to be ready - read the status reg until read_if_ready bit is set to 1 AND maximum wait time of 50ms */
			usleep_range(1000,2000); /* No details in specs regarding how much time we should wait for this. Hence a guess, try with 1ms intervals :) */
			rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_read(
			&(e_ctrl->i2c_client), CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_STATUS,
			&status_reg_val,MSM_CAMERA_I2C_BYTE_DATA);
		}
		CDBG(" DTI: Polling required for reading done.\n");
	}

	/* Check if there were any errors reported in the transfer */
	if(rc < 0) {
		rc = -1;
		pr_err("ERROR: DTI: polling i2c read failed\n");
	} else if (status_reg_val & 0x0C) {
		if (status_reg_val & 0x08) {
			rc = -1;
			pr_err("ERROR: DTI: IMPROPER INTERFACE USAGE. NVM/OTP contents will be erroneous\n");
		}
		if (status_reg_val & 0x04) {
			rc = -1;
			pr_err("ERROR: DTI: DATA CORRUPTED. NVM/OTP contents will be erroneous\n");
		}
		/* Clear the error bits */
			e_ctrl->i2c_client.i2c_func_tbl->i2c_write(
			&(e_ctrl->i2c_client), CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_CTRL,
			0x04,MSM_CAMERA_I2C_BYTE_DATA);
	} else if (!max_trials_for_polling) {
		rc = -1;
		pr_err("ERROR: DTI: Maximum time of 50ms to wait for the read interface to get ready exceeded. NVM/OTP contents will be erroneous\n");
	} else {
		rc = 0;
	}
	return rc;

}


static int32_t read_eeprom_memory(struct msm_eeprom_ctrl_t *e_ctrl)
{
	int rc = 0;
	CAM_DRV_SMIApp_DATA_UPLOAD_DOWNLOAD_IF_T data_upload_download_interface;
	uint8_t *p_nvm_data_temp = NULL;
	uint16_t checksum_addr = 0;

	if (!e_ctrl) {
		pr_err("%s e_ctrl is NULL", __func__);
		return -1;
	}

	rc = smiapp_eeprom_read_data_interface(e_ctrl, &data_upload_download_interface);
	if (rc < 0) {
		return rc;
	}

	/* Check and prepare the Data Interface for downloading the NVM data from the sensor */
	if (data_upload_download_interface.if_1_supported) {
		uint16_t current_page_no, max_page_no;
		current_page_no = max_page_no = 0;

		rc = smiapp_eeprom_select_page(e_ctrl, 0);
		if (rc < 0) {
			return rc;
		}
		rc = smiapp_eeprom_interface_enable(e_ctrl, 1);
		if (rc < 0) {
			return rc;
		}

		rc = smiapp_eeprom_waiting_data_ready(e_ctrl, data_upload_download_interface.polling_needed_in_reading);
		if (rc < 0) {
			return rc;
		}
		if (e_ctrl->memory_data == NULL) {
			uint8_t initial_nvm_data[5];
			/* Now we need to parse the checksum address from the NVM data as this will give an indication of the amount of data.
			The checksum will come from mem_addr_register. This register can either be at 0x02 or 0x03 location of NVM depending on
			config_2 register being absent or present respectively. The presence of config_2 register can be decoded from config_1
			register. */

			/* First read the initial 5 bytes (0 - 4) and hence make sure we have the config_1, config_2 (if exists) and mem_addr_reg values */
			rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_read_seq(
				&(e_ctrl->i2c_client), CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_DATA_0,
				initial_nvm_data,5);

			if(rc > 0) {
				uint8_t mem_addr_reg;
				/* Decide the address of mem_addr reg based on config_2 reg's existence */
				mem_addr_reg = (initial_nvm_data[CAM_SMIApp_NVM_CONFIG_1_REG_ADDR] & 0x08) == 0x08 ? CAM_SMIApp_NVM_CONFIG_1_REG_ADDR + 2 :
				                                                                  CAM_SMIApp_NVM_CONFIG_1_REG_ADDR + 1;

				/* Now decode the checksum address using mem_addr_reg */
				checksum_addr = ((uint16_t)initial_nvm_data[mem_addr_reg] << 5) | (initial_nvm_data[mem_addr_reg+1] >> 3 & 0x1F);
				max_page_no = (uint8_t) ( (checksum_addr+ CAM_SMIApp_NVM_BYTES_PER_PAGE -1)/ CAM_SMIApp_NVM_BYTES_PER_PAGE);
				CDBG("NVM: Checksum address: 0x%x, No of NVM pages to read: %d \n", checksum_addr, max_page_no);

				/* Check max_page_no is > 0*/
				if(max_page_no > 0) {
					/* Now allocate the required memory based on the number of pages to be read */
					e_ctrl->memory_data = kzalloc((max_page_no * CAM_SMIApp_NVM_BYTES_PER_PAGE), GFP_KERNEL);
					if (e_ctrl->memory_data != NULL) {
						p_nvm_data_temp = e_ctrl->memory_data;
					}
					else {
						rc = -1;
						pr_err("ERROR: NVM: Failed to allocate memory for NVM/OTP contents.\n");
					}
				} else {
					pr_err("ERROR: NVM: max_page_no equal to 0 \n");
					rc = -1;
				}
			}
		}


		while((current_page_no < max_page_no) && (rc >= 0)) {
			CDBG(" NVM: Reading page no: %d\n", current_page_no);
			rc = smiapp_eeprom_select_page(e_ctrl, current_page_no);
			if (rc < 0) {
				break;
			}

			rc = smiapp_eeprom_waiting_data_ready(e_ctrl, data_upload_download_interface.polling_needed_in_reading);
			if (rc < 0) {
				break;
			}

			/* read one page data */
			if (p_nvm_data_temp != NULL){
				// Page read is changed to 16 byte reads due to issues seen with some sensors
				rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_read_seq(
				&(e_ctrl->i2c_client), CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_DATA_0,
				p_nvm_data_temp,16);

				rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_read_seq(
				&(e_ctrl->i2c_client), CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_DATA_0+16,
				p_nvm_data_temp+16,16);

				rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_read_seq(
				&(e_ctrl->i2c_client), CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_DATA_0+32,
				p_nvm_data_temp+32,16);

				rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_read_seq(
				&(e_ctrl->i2c_client), CAM_SMIApp_REG8_DATA_UPLOAD_DOWNLOAD_IF1_DATA_0+48,
				p_nvm_data_temp+48,16);

				p_nvm_data_temp += CAM_SMIApp_NVM_BYTES_PER_PAGE;

			}
			current_page_no++;

		}

		rc = smiapp_eeprom_interface_enable(e_ctrl, 0);;     /* All done. Hence disable the interface */

		if (rc < 0) {
			pr_err("ERROR: NVM: Failed to read NVM\n");
			/* Error has occured. Hence dealloc any NVM buffer */
			if(e_ctrl->memory_data != NULL) {
				kfree(e_ctrl->memory_data);
				e_ctrl->memory_data = NULL; /* Could point to garbage value even after deallocation, hence set to NULL explicitly as a good practice */
			}
		} else {
			e_ctrl->cal_data.num_data = checksum_addr+3;

			/* check if DCC have DTI overrides */
			//cam_drv_SMIApp_get_dti_override_from_dcc((uint8*)dcc_register_blocks[CAM_DRV_DCC_REGISTER_BLOCK_SW_CONFIG_DATA].p_config_register_block);

			CDBG("NVM reading completed successfully\n");
			CDBG("Page NVM Address     Data\n");

			p_nvm_data_temp = e_ctrl->memory_data; /* Realign to the start of NVM data */
			if(checksum_addr && p_nvm_data_temp != NULL) {
				uint32_t checksum =0;
				uint32_t checksum_calculated =0;
				checksum = p_nvm_data_temp[checksum_addr]<<8 | p_nvm_data_temp[checksum_addr+1];
				checksum_calculated= nvm_calculate_checsum(p_nvm_data_temp,checksum_addr);
				CDBG("checksum data:%02x,%02x",p_nvm_data_temp[checksum_addr],p_nvm_data_temp[checksum_addr+1]);
				CDBG("checksum :%04x,%04x",checksum,checksum_calculated);
				if(checksum != checksum_calculated) {
					kfree(e_ctrl->memory_data);
					e_ctrl->memory_data = NULL; /* Could point to garbage value even after deallocation, hence set to NULL explicitly as a good practice */
					rc = -1;
				}
			}
		}
	} else {
		rc = -1;
		pr_err("ERROR: Data upload/download interface not supported. Cannot read NVM/OTP data\n");
	}
	return rc;
}

static int msm_eeprom_get_dt_data(struct msm_eeprom_ctrl_t *e_ctrl)
{
	int rc = 0, i = 0;
	struct msm_eeprom_board_info *eb_info;
	struct msm_camera_power_ctrl_t *power_info =
		&e_ctrl->eboard_info->power_info;
	struct device_node *of_node = NULL;
	struct msm_camera_gpio_conf *gconf = NULL;
	uint16_t gpio_array_size = 0;
	uint16_t *gpio_array = NULL;

	eb_info = e_ctrl->eboard_info;
	if (e_ctrl->eeprom_device_type == MSM_CAMERA_SPI_DEVICE)
		of_node = e_ctrl->i2c_client.
			spi_client->spi_master->dev.of_node;
	else if (e_ctrl->eeprom_device_type == MSM_CAMERA_PLATFORM_DEVICE)
		of_node = e_ctrl->pdev->dev.of_node;
	else if (e_ctrl->eeprom_device_type == MSM_CAMERA_I2C_DEVICE)
		of_node = e_ctrl->i2c_client.client->dev.of_node;

	rc = msm_camera_get_dt_vreg_data(of_node, &power_info->cam_vreg,
						&power_info->num_vreg);
	if (rc < 0)
		return rc;

	rc = msm_camera_get_dt_power_setting_data(of_node,
		power_info->cam_vreg, power_info->num_vreg,
		&power_info->power_setting, &power_info->power_setting_size);
	if (rc < 0)
		goto error1;

	power_info->gpio_conf = kzalloc(sizeof(struct msm_camera_gpio_conf),
					GFP_KERNEL);
	if (!power_info->gpio_conf) {
		rc = -ENOMEM;
		goto error2;
	}
	gconf = power_info->gpio_conf;
	gpio_array_size = of_gpio_count(of_node);
	CDBG("%s gpio count %d\n", __func__, gpio_array_size);

	if (gpio_array_size) {
		gpio_array = kzalloc(sizeof(uint16_t) * gpio_array_size,
			GFP_KERNEL);
		if (!gpio_array) {
			pr_err("%s failed %d\n", __func__, __LINE__);
			goto error3;
		}
		for (i = 0; i < gpio_array_size; i++) {
			gpio_array[i] = of_get_gpio(of_node, i);
			CDBG("%s gpio_array[%d] = %d\n", __func__, i,
				gpio_array[i]);
		}

		rc = msm_camera_get_dt_gpio_req_tbl(of_node, gconf,
			gpio_array, gpio_array_size);
		if (rc < 0) {
			pr_err("%s failed %d\n", __func__, __LINE__);
			goto error4;
		}

		rc = msm_camera_init_gpio_pin_tbl(of_node, gconf,
			gpio_array, gpio_array_size);
		if (rc < 0) {
			pr_err("%s failed %d\n", __func__, __LINE__);
			goto error4;
		}
		kfree(gpio_array);
	}

	return rc;
error4:
	kfree(gpio_array);
error3:
	kfree(power_info->gpio_conf);
error2:
	kfree(power_info->power_setting);
error1:
	kfree(power_info->cam_vreg);
	return rc;
}


static struct msm_cam_clk_info cam_8960_clk_info[] = {
	[SENSOR_CAM_MCLK] = {"cam_clk", 24000000},
};

static struct v4l2_subdev_core_ops msm_eeprom_subdev_core_ops = {
	.ioctl = msm_eeprom_subdev_ioctl,
};

static struct v4l2_subdev_ops msm_eeprom_subdev_ops = {
	.core = &msm_eeprom_subdev_core_ops,
};

static int32_t msm_eeprom_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id) {
	int rc = 0;
	uint32_t temp = 0;
	struct msm_eeprom_ctrl_t *e_ctrl = NULL;
	struct msm_camera_power_ctrl_t *power_info = NULL;
	struct device_node *of_node = client->dev.of_node;
	CDBG("%s E\n", __func__);
	if (!of_node) {
		pr_err("%s of_node NULL\n", __func__);
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s i2c_check_functionality failed\n", __func__);
		goto probe_failure;
	}

	e_ctrl = kzalloc(sizeof(struct msm_eeprom_ctrl_t), GFP_KERNEL);
	if (!e_ctrl) {
		pr_err("%s:%d kzalloc failed\n", __func__, __LINE__);
		return -ENOMEM;
	}
	e_ctrl->eeprom_v4l2_subdev_ops = &msm_eeprom_subdev_ops;
	e_ctrl->eeprom_mutex = &msm_eeprom_mutex;
	CDBG("%s client = %x\n", __func__, (unsigned int)client);
	e_ctrl->eboard_info = kzalloc(sizeof(
		struct msm_eeprom_board_info), GFP_KERNEL);
	if (!e_ctrl->eboard_info) {
		pr_err("%s:%d board info NULL\n", __func__, __LINE__);
		rc = -EINVAL;
		goto e_ctrl_free;
	}

	rc = of_property_read_u32(of_node, "qcom,slave-addr", &temp);
	if (rc < 0) {
		pr_err("%s failed rc %d\n", __func__, rc);
		goto board_free;
	}

	power_info = &e_ctrl->eboard_info->power_info;
	e_ctrl->eboard_info->i2c_slaveaddr = temp;
	e_ctrl->i2c_client.client = client;
	e_ctrl->is_supported = 0;

	/* Set device type as I2C */
	e_ctrl->eeprom_device_type = MSM_CAMERA_I2C_DEVICE;
	e_ctrl->i2c_client.i2c_func_tbl = &msm_eeprom_qup_func_tbl;

	if (e_ctrl->eboard_info->i2c_slaveaddr != 0)
		e_ctrl->i2c_client.client->addr =
					e_ctrl->eboard_info->i2c_slaveaddr;
	power_info->clk_info = cam_8960_clk_info;
	power_info->clk_info_size = ARRAY_SIZE(cam_8960_clk_info);
	power_info->dev = &client->dev;
    e_ctrl->i2c_client.addr_type = MSM_CAMERA_I2C_WORD_ADDR;
    CDBG("client->addr %02x, addr_type %d\n",e_ctrl->i2c_client.client->addr,
            e_ctrl->i2c_client.addr_type);

	rc = of_property_read_string(of_node, "qcom,eeprom-name",
		&e_ctrl->eboard_info->eeprom_name);
	CDBG("%s qcom,eeprom-name %s, rc %d\n", __func__,
		e_ctrl->eboard_info->eeprom_name, rc);
	if (rc < 0) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		goto board_free;
	}

	rc = msm_eeprom_get_dt_data(e_ctrl);
	if (rc)
		goto board_free;


	rc = msm_camera_power_up(power_info, e_ctrl->eeprom_device_type,
		&e_ctrl->i2c_client);
	if (rc) {
		pr_err("%s failed power up %d\n", __func__, __LINE__);
		goto dt_data_free;
	}


	rc = read_eeprom_memory(e_ctrl);
	if (rc < 0) {
		pr_err("%s read_eeprom_memory failed\n", __func__);
		goto power_down;
	}

	rc = msm_camera_power_down(power_info, e_ctrl->eeprom_device_type,
		&e_ctrl->i2c_client);
	if (rc) {
		pr_err("failed rc %d\n", rc);
		goto memdata_free;
	}

	/*IMPLEMENT READING PART*/
	/* Initialize sub device */
	v4l2_i2c_subdev_init(&e_ctrl->msm_sd.sd,
		e_ctrl->i2c_client.client,
		e_ctrl->eeprom_v4l2_subdev_ops);
	v4l2_set_subdevdata(&e_ctrl->msm_sd.sd, e_ctrl);
	e_ctrl->msm_sd.sd.internal_ops = &msm_eeprom_internal_ops;
	e_ctrl->msm_sd.sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	media_entity_init(&e_ctrl->msm_sd.sd.entity, 0, NULL, 0);
	e_ctrl->msm_sd.sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	e_ctrl->msm_sd.sd.entity.group_id = MSM_CAMERA_SUBDEV_EEPROM;
	msm_sd_register(&e_ctrl->msm_sd);
	e_ctrl->is_supported = 1;
	CDBG("%s success result=%d X\n", __func__, rc);
	return rc;



power_down:
	msm_camera_power_down(power_info, e_ctrl->eeprom_device_type,
		&e_ctrl->i2c_client);

memdata_free:
	if(e_ctrl->memory_data)
		kfree(e_ctrl->memory_data);

dt_data_free:
	kfree(e_ctrl->eboard_info->power_info.gpio_conf);
	kfree(e_ctrl->eboard_info->power_info.power_setting);
	kfree(e_ctrl->eboard_info->power_info.cam_vreg);

board_free:
	kfree(e_ctrl->eboard_info);

e_ctrl_free:
	kfree(e_ctrl);

probe_failure:
	pr_err("%s failed! rc = %d\n", __func__, rc);
	return rc;
}

static int32_t msm_eeprom_i2c_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct msm_eeprom_ctrl_t  *e_ctrl;
	if (!sd) {
		pr_err("%s: Subdevice is NULL\n", __func__);
		return 0;
	}

	e_ctrl = (struct msm_eeprom_ctrl_t *)v4l2_get_subdevdata(sd);
	if (!e_ctrl) {
		pr_err("%s: eeprom device is NULL\n", __func__);
		return 0;
	}

	kfree(e_ctrl->memory_data);
	if (e_ctrl->eboard_info) {
		kfree(e_ctrl->eboard_info->power_info.gpio_conf);
		kfree(e_ctrl->eboard_info->power_info.power_setting);
		kfree(e_ctrl->eboard_info->power_info.cam_vreg);
		kfree(e_ctrl->eboard_info);
	}

	kfree(e_ctrl);
	return 0;
}


static const struct of_device_id msm_eeprom_i2c_dt_match[] = {
	{.compatible = "qcom,smiapp_eeprom"},
	{}
};

MODULE_DEVICE_TABLE(of, msm_eeprom_i2c_dt_match);

static const struct i2c_device_id msm_eeprom_i2c_id[] = {
	{ "smiapp_eeprom", (kernel_ulong_t)NULL},
	{ }
};

static struct i2c_driver msm_eeprom_i2c_driver = {
	.id_table = msm_eeprom_i2c_id,
	.probe  = msm_eeprom_i2c_probe,
	.remove = __devexit_p(msm_eeprom_i2c_remove),
	.driver = {
		.name = "smiapp_eeprom",
		.of_match_table = msm_eeprom_i2c_dt_match,
	},
};

static int __init msm_eeprom_init_module(void)
{
	CDBG("%s E\n", __func__);
	return i2c_add_driver(&msm_eeprom_i2c_driver);
}

static void __exit msm_eeprom_exit_module(void)
{
	i2c_del_driver(&msm_eeprom_i2c_driver);
}

module_init(msm_eeprom_init_module);
module_exit(msm_eeprom_exit_module);
MODULE_DESCRIPTION("MSM EEPROM driver");
MODULE_LICENSE("GPL v2");