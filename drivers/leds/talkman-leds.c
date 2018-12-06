#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/leds.h>

#define FLASHLIGHT_I2C_ADDR 0x10  // Replace with actual I2C address
#define FLASHLIGHT_REG_CONTROL 0x01  // Replace with actual register address for control

// Number of LEDs
#define NUM_LEDS 3

// LED control bit positions in the register
#define LED1_BIT 0
#define LED2_BIT 1
#define LED3_BIT 2

struct flashlight_data {
    struct led_classdev cdev[NUM_LEDS];
    struct i2c_client *client;
};

static void flashlight_set(struct led_classdev *led_cdev, enum led_brightness value)
{
    struct flashlight_data *flashlight = container_of(led_cdev, struct flashlight_data, cdev[0]);
    u8 data;
    int led_index = (led_cdev - &flashlight->cdev[0]) / sizeof(struct led_classdev);

    // Read the current state of the register
    data = i2c_smbus_read_byte_data(flashlight->client, FLASHLIGHT_REG_CONTROL);
    if (data < 0) {
        dev_err(&flashlight->client->dev, "Failed to read control register\n");
        return;
    }

    // Set or clear the bit for the current LED
    if (value == LED_OFF) {
        data &= ~(1 << led_index);  // Clear the bit
    } else {
        data |= (1 << led_index);   // Set the bit
    }

    // Write the updated state to the register
    if (i2c_smbus_write_byte_data(flashlight->client, FLASHLIGHT_REG_CONTROL, data) < 0) {
        dev_err(&flashlight->client->dev, "Failed to write control register\n");
    }
}

static int flashlight_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct flashlight_data *flashlight;
    int ret, i;

    flashlight = kzalloc(sizeof(*flashlight), GFP_KERNEL);
    if (!flashlight)
        return -ENOMEM;

    flashlight->client = client;

    for (i = 0; i < NUM_LEDS; i++) {
        flashlight->cdev[i].name = kasprintf(GFP_KERNEL, "flashlight-%d", i);
        flashlight->cdev[i].brightness_set = flashlight_set;
        flashlight->cdev[i].max_brightness = LED_FULL;
        ret = devm_led_classdev_register(&client->dev, &flashlight->cdev[i]);
        if (ret) {
            dev_err(&client->dev, "Failed to register LED device %d\n", i);
            while (i--)
                devm_led_classdev_unregister(&client->dev, &flashlight->cdev[i]);
            kfree(flashlight);
            return ret;
        }
    }

    return 0;
}

static int flashlight_remove(struct i2c_client *client)
{
    // Cleanup done by devm_led_classdev_unregister during probe failure
    return 0;
}

static const struct i2c_device_id flashlight_id[] = {
    { "flashlight", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, flashlight_id);

static const struct of_device_id flashlight_of_match[] = {
    { .compatible = "my-flashlight", },
    { }
};
MODULE_DEVICE_TABLE(of, flashlight_of_match);

static struct i2c_driver flashlight_driver = {
    .driver = {
        .name = "flashlight",
        .of_match_table = flashlight_of_match,
    },
    .probe = flashlight_probe,
    .remove = flashlight_remove,
    .id_table = flashlight_id,
};

module_i2c_driver(flashlight_driver);

MODULE_AUTHOR("ItzzJuliann");
MODULE_DESCRIPTION("Lumia 950 Flashlight Driver");
MODULE_LICENSE("GPL");
