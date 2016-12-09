#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <linux/of_gpio.h>
#ifdef CONFIG_OF //Open firmware must be defined for dts useage
static struct of_device_id qcom_spi_test_table[] = {
	{ .compatible = "qcom,spi-test",}, //Compatible node must match
	//dts
	{ },
};
#else
#define qcom_spi_test_table NULL
#endif
#define BUFFER_SIZE 4<<10
struct spi_message spi_msg;
struct spi_transfer spi_xfer;
u8 tx_buf[]= { //This needs to be DMA friendly buffer
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x40, 0x00, 0x00, 0x00, 0x00, 0x95,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xAD,
		0xF0, 0x0D,
	};
u8 rx_buf[ARRAY_SIZE(tx_buf)] = {0, };
static int spi_test_transfer(struct spi_device *spi)
{
	int ret;
	spi->mode |=SPI_MODE_0; //Enable Loopback mode
	spi_message_init(&spi_msg);
	spi_xfer.tx_buf = &tx_buf;
	spi_xfer.rx_buf = &rx_buf;
	spi_xfer.len = ARRAY_SIZE(tx_buf);
	spi_xfer.bits_per_word = 8;
	/*
	 * Spi master driver will round the speed_hz to the lower nearest
	 frequency in the spi clock table ftbl_blsp*_qup*_spi_apps_clk_src[]
	 with kernel/drivers/clk/qcom/clock-gcc-(chipset).c
	 */
	spi_xfer.speed_hz = spi->max_speed_hz;
	spi_message_add_tail(&spi_xfer, &spi_msg);
	ret = spi_sync(spi, &spi_msg);
	printk("%s ----> spi sync length:%d\n",__func__,spi_msg.actual_length);
	for (ret = 0; ret < ARRAY_SIZE(tx_buf); ret++) {
		if (!(ret % 6))
			printk(" ");
		printk("%.2X ", rx_buf[ret]);
	}
	return ret;
}
static int spi_test_probe(struct spi_device *spi)
{
	int irq, irq_gpio, cs, cpha, cpol, cs_high, max_speed;
	//int i;
	//allocate memory for transfer
	
	//tx_buf = kmalloc(BUFFER_SIZE, GFP_ATOMIC);
	printk(KERN_INFO"spi_test_probe begin\n");
	if(tx_buf == NULL){
		dev_err(&spi->dev, "%s: mem alloc failed\n", __func__);
		return -ENOMEM;
	}
	//Parse data using dt.
	if(spi->dev.of_node){
		irq_gpio = of_get_named_gpio_flags(spi->dev.of_node,
				"qcom_spi_test,irq-gpio", 0, NULL);
	}
	irq = spi->irq;
	cs = spi->chip_select;
	cpha = ( spi->mode & SPI_CPHA ) ? 1:0;
	cpol = ( spi->mode & SPI_CPOL ) ? 1:0;
	cs_high = ( spi->mode & SPI_CS_HIGH ) ? 1:0;
	max_speed = spi->max_speed_hz;
	printk(KERN_INFO"gpio [%d] irq [%d] gpio_irq [%d] cs [%x] CPHA[%x] CPOL [%x] CS_HIGH [%x]\n",
			irq_gpio, irq, gpio_to_irq(irq_gpio), cs, cpha, cpol,
			cs_high);
	printk(KERN_INFO"Max_speed [%d]\n", max_speed );
	//Transfer can be done after spi_device structure is created
	spi->bits_per_word = 8;
	//for (i = 0;i < 50;i++) {
	printk(KERN_INFO"SPI sync returned [%d]\n",
			spi_test_transfer(spi));
	//}
	return 0;
}
//SPI Driver Info
static struct spi_driver spi_test_driver = {
	.driver = {
		.name = "qcom_spi_test",
		.owner = THIS_MODULE,
		.of_match_table = qcom_spi_test_table,
	},
	.probe = spi_test_probe,
};
static int __init spi_test_init(void)
{
	return spi_register_driver(&spi_test_driver);
}
static void __exit spi_test_exit(void)
{
	spi_unregister_driver(&spi_test_driver);
}
module_init(spi_test_init);
module_exit(spi_test_exit);
MODULE_DESCRIPTION("SPI TEST");
MODULE_LICENSE("GPL v2");
