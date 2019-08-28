#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>

struct gpio_bitbank_plat_data {
	struct gpio_descs *input_gpios;  
};


static int gpio_bitbank_get_gpios_state(struct device *dev, struct gpio_bitbank_plat_data *pdata) {
	struct gpio_descs *gpios = pdata->input_gpios;
	unsigned int i, ret = 0;
	int val;
	
	for (i = 0; i < 4; ++i) {
		val = gpiod_get_value_cansleep(gpios->desc[i]);
		if (val < 0) {
			dev_err(dev, "Error %d reading gpio %d.\n", val, desc_to_gpio(gpios->desc[i]));
			return val;
		}

		val = !!val;
		ret = (ret << 1) | val;
	}

	return ret;
}


static ssize_t gpio_bitbank_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	struct device* dev = (struct device*)kobj;
	struct gpio_bitbank_plat_data *pdata = dev_get_drvdata(dev);

	int gpioval = gpio_bitbank_get_gpios_state(dev, pdata);
	return sprintf(buf, "%d\n", gpioval);
}

static struct kobj_attribute gpio_bitbank_attribute =__ATTR(value, 0444, gpio_bitbank_show, NULL);


static int gpio_bitbank_probe(struct platform_device *pdev) {
	struct device *dev = &pdev->dev;
	struct gpio_bitbank_plat_data *pdata = devm_kzalloc(dev, sizeof(struct gpio_bitbank_plat_data), GFP_KERNEL);
	int ret;
	
	if (!pdata) {
		dev_err(dev, "Unable to allocate pdata.\n");
		return -ENOMEM;
	}

	dev_set_drvdata(dev, pdata);

	pdata->input_gpios = devm_gpiod_get_array(dev, NULL, GPIOD_IN);

	if (IS_ERR(pdata->input_gpios)) {
		ret = PTR_ERR(pdata->input_gpios);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Unable to acquire gpios.\n");
		return ret;
	}

	if (sysfs_create_file(&pdev->dev.kobj, &gpio_bitbank_attribute.attr)) {
		dev_err(dev, "Failed to create sysfs file for gpio-bitbank.\n");
		return -EBUSY;
	}
	
	return 0;
}


static const struct of_device_id of_gpio_bitbank_match[] = {
	{ .compatible = "gpio-bitbank", },
	{},
};


static struct platform_driver gpio_bitbank_driver = {
	.probe		= gpio_bitbank_probe,
	.driver		= {
		.name	= "gpio-bitbank",
		.of_match_table = of_match_ptr(of_gpio_bitbank_match),
	},
};


module_platform_driver(gpio_bitbank_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Skeffington");
MODULE_DESCRIPTION("gpio bit bank driver.");
MODULE_VERSION("1");
