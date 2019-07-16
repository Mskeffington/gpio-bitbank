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
	struct device *dev;
	const char *label;
	struct gpio_descs *input_gpios;  
};


static int gpio_bitbank_get_gpios_state(struct gpio_bitbank_plat_data *pdata) {
	struct gpio_descs *gpios = pdata->input_gpios;
	unsigned int ret = 0;
	int i, val;

	for (i = 0; i < gpios->ndescs; i++) {
		val = gpiod_get_value_cansleep(gpios->desc[i]);
		if (val < 0) {
			dev_err(pdata->dev, "Error %d reading gpio %d.\n", val, desc_to_gpio(gpios->desc[i]));
			return val;
		}

		val = !!val;
		ret = (ret << 1) | val;
	}

	return ret;
}

static ssize_t gpio_bitbank_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	struct platform_device *pdev = to_platform_device(container_of(kobj, struct device, kobj));
	struct gpio_bitbank_plat_data *pdata = platform_get_drvdata(pdev);
    
	int gpioval = gpio_bitbank_get_gpios_state(pdata);
	return sprintf(buf, "%d\n", gpioval);
}

static struct kobj_attribute gpio_bitbank_attribute =__ATTR(value, 0444, gpio_bitbank_show, NULL);


static int gpio_bitbank_probe(struct platform_device *pdev) {
    
	struct device *dev = &pdev->dev;
	struct gpio_bitbank_plat_data *pdata = devm_kzalloc(dev, sizeof(struct gpio_bitbank_plat_data), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "Unable to allocate pdata.\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, pdata);

	/* currently unused */
	device_property_read_string(dev, "label", &pdata->label);
	pdata->input_gpios = devm_gpiod_get_array(dev, NULL, GPIOD_IN);

	if (IS_ERR(pdata->input_gpios)) {
		dev_err(dev, "Unable to acquire gpios.\n");
		return PTR_ERR(pdata->input_gpios);
	}
	if (pdata->input_gpios->ndescs < 2) {
		dev_err(dev, "Not enough gpios provided.\n");
		return -EINVAL;
	}
	
	pdata->dev = dev;
    
	if (sysfs_create_file(&pdev->dev.kobj, &gpio_bitbank_attribute.attr)) {
		dev_err(dev, "Failed to create sysfs file for gpio-bitbank.\n");
		return -EBUSY;
	}
	return 0;
}

static void __exit gpio_bitbank_exit(struct platform_device *pdev) {
	kobject_put(&pdev->dev.kobj);
}

static const struct of_device_id of_gpio_bitbank_match[] = {
	{ .compatible = "gpio-bitbank", },
	{},
};

static struct platform_driver gpio_bitbank_driver = {
	.probe		= gpio_bitbank_probe,
	.shutdown	= gpio_bitbank_exit,
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
