/**
 * SHOOTERX1 <yorha.a2@foxmail.com>
 */
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/compat.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include "epaper_cmds.h"

#define EPAPER_SPI_MAJOR 225
#define N_SPI_MINORS 2

static struct class *epaper_class;
static unsigned bufsiz = 4096;
module_param(bufsiz, uint, S_IRUGO);

struct epaper_drv_data {
    dev_t                   devt;
    struct spi_device       *spi;
    u8                      *tx_buffer;
    u8                      *rx_buffer;
    spinlock_t              spi_lock;
    struct mutex            buf_lock;
    unsigned                users;
    u32                     speed_hz;
    int                     reset_gpio;
    int                     busy_gpio;
    int                     dc_gpio;
};

static struct epaper_drv_data *epaper_drv_data_p;

static const struct of_device_id epaper_dt_ids[] = {                      
    { .compatible = "epaper" },
    {},
};
MODULE_DEVICE_TABLE(of, epaper_dt_ids);

/*---------------------------------------------------------------------------*/
static ssize_t
epaper_sync(struct epaper_drv_data *epd, struct spi_message *message)
{
    DECLARE_COMPLETION_ONSTACK(done);
    int status;
    struct spi_device *spi;
    
    spin_lock_irq(&epd->spi_lock);
    spi = epd->spi;
    spin_unlock_irq(&epd->spi_lock);

    if(spi == NULL)
        status = -ESHUTDOWN;
    else
        status = spi_sync(spi, message);

    if (status == 0)
        status = message->actual_length;

    return status;
}

static inline ssize_t
epaper_sync_write(struct epaper_drv_data *epd, size_t len)
{
    struct spi_transfer t = {
        .tx_buf     = epd->tx_buffer,
        .len        = len,
        .speed_hz   = epd->speed_hz,
    };
    struct spi_message m;

    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    return epaper_sync(epd, &m);
}

static inline ssize_t
epaper_sync_read(struct epaper_drv_data *epd, size_t len)
{
    struct spi_transfer t = {
        .rx_buf     = epd->rx_buffer,
        .len        = len,
        .speed_hz   = epd->speed_hz,
    };
    struct spi_message m;

    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    return epaper_sync(epd, &m);
}

static ssize_t
epaper_write(struct file *filp, const char __user *buf,
        size_t count, loff_t *f_pos)
{
    struct epaper_drv_data *epd;
    ssize_t             status = 0;
    unsigned long          missing;

    if(count > bufsiz)
        return -EMSGSIZE;
    
    epd = filp->private_data;

    mutex_lock(&epd->buf_lock);
    missing = copy_from_user(epd->tx_buffer, buf, count);
    if(missing == 0)
        status = epaper_sync_write(epd, count);
    else
        status = -EFAULT;
    mutex_unlock(&epd->buf_lock);

    return status;
}

static ssize_t
epaper_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    /* @@ should't use this function @@ */
    /* @@ use for debug @@*/
    struct epaper_drv_data *epd;
    ssize_t          status = 0;

    if (count > bufsiz)
        return -EMSGSIZE;
    
    epd = filp->private_data;

    mutex_lock(&epd->buf_lock);
    status = epaper_sync_read(epd, count);
    if (status > 0) {
        unsigned long missing;
        missing = copy_to_user(buf, epd->rx_buffer, status);
        if (missing == status)
            status = -EFAULT;
        else
            status = status - missing;
    }
    mutex_unlock(&epd->buf_lock);

    return status;
}

static int epaper_open(struct inode *inode, struct file *filp)
{
    struct epaper_drv_data *epd;
    int            status = -ENXIO;

    epd = epaper_drv_data_p;

    if(!epd->tx_buffer) {
        epd->tx_buffer = kmalloc(bufsiz, GFP_KERNEL);
        if (!epd->tx_buffer) {
            dev_dbg(&epd->spi->dev, "open/ENOMEM\n");
            status = -ENOMEM;
            goto err_alloc_tx_buf;
        }
    }

    if(!epd->rx_buffer) {
        epd->rx_buffer = kmalloc(bufsiz, GFP_KERNEL);
        if (!epd->rx_buffer) {
            dev_dbg(&epd->spi->dev, "open/ENOMEM\n");
            status = -ENOMEM;
            goto err_alloc_rx_buf;
        }
    }

    epd->users++;
    filp->private_data = epd;
    nonseekable_open(inode, filp);

    return 0;

err_alloc_rx_buf:
    kfree(epd->tx_buffer);
    epd->tx_buffer = NULL;
err_alloc_tx_buf:
    return status;
}

static int epaper_release(struct inode *inode, struct file *filp)
{
    struct epaper_drv_data *epd;

    epd = filp->private_data;
    filp->private_data = NULL;

    epd->users--;
    if(!epd->users) {
        int     dofree;

        kfree(epd->tx_buffer);
        epd->tx_buffer = NULL;

        kfree(epd->rx_buffer);
        epd->rx_buffer = NULL;

        spin_lock_irq(&epd->spi_lock);
        if(epd->spi)
            epd->speed_hz = epd->spi->max_speed_hz;
        
        dofree = (epd->spi == NULL);
        spin_unlock_irq(&epd->spi_lock);

        if(dofree)
            kfree(epd);
    }

    return 0;
}

static long
epaper_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int                 retval = 0;
    struct epaper_drv_data    *epd;

    if (_IOC_TYPE(cmd) != EPAPER_MAGIC)
        return -ENOTTY;

    epd = filp->private_data;

    mutex_lock(&epd->buf_lock);
    switch (cmd)
    {
        case EPAPER_IS_DEV_BUSY:
            retval = gpio_get_value(epd->busy_gpio);
            break;
        case EPAPER_DC_PIN_SET_HIGH:
            gpio_set_value(epd->dc_gpio, 1);
            break;
        case EPAPER_DC_PIN_SET_LOW:
            gpio_set_value(epd->dc_gpio, 0);
            break;
        case EPAPER_RESET:
            gpio_set_value(epd->reset_gpio, 0);
            msleep(200);
            gpio_set_value(epd->reset_gpio, 1);
            msleep(200);
            break;
        default:
            retval = -EINVAL;
            break;
    }
    mutex_unlock(&epd->buf_lock);

    return retval;
}

static const struct file_operations epaper_fops = {
    .owner = THIS_MODULE,
    .write = epaper_write,
    .unlocked_ioctl = epaper_ioctl,
    .read = epaper_read,
    .open = epaper_open,
    .release  = epaper_release,
    .llseek = no_llseek,
};
/*--------------------------------------------------------------------------*/
static int epaper_spi_probe(struct spi_device *spi)
{
    struct epaper_drv_data      *epd;
    int                         err;
    struct gpio_desc            *busy,
                                *reset,
                                *dc;
    struct device               *dev;

    spi->bits_per_word = 8;
    spi->mode = SPI_MODE_0;
    err = spi_setup(spi);
    if(err < 0)
        return err;
    epd = kzalloc(sizeof(struct epaper_drv_data), GFP_KERNEL);
    if(!epd)
        return -ENOMEM;
    epd->spi = spi;
    spin_lock_init(&epd->spi_lock);
    mutex_init(&epd->buf_lock);
    epd->devt = MKDEV(EPAPER_SPI_MAJOR, 0);
    dev = device_create(epaper_class, &spi->dev, epd->devt,
                            epd, "epaper_spi_dev");
    printk("%s:name=%s,bus_num=%d,cs=%d,mode=%d,speed=%d\n",__func__,
            spi->modalias, spi->master->bus_num, spi->chip_select,
            spi->mode, spi->max_speed_hz);
    busy    = devm_gpiod_get(&spi->dev, "busy", GPIOD_IN);
    dc      = devm_gpiod_get(&spi->dev, "dc", GPIOD_OUT_HIGH);
    reset   = devm_gpiod_get(&spi->dev, "reset", GPIOD_OUT_HIGH);
    if(!busy && !dc && !reset) {
        err = -ENODEV;
        goto out;
    }
    epd->busy_gpio   = desc_to_gpio(busy);
    epd->dc_gpio     = desc_to_gpio(dc);
    epd->reset_gpio  = desc_to_gpio(reset);
out:
    if (err == 0) {
        spi_set_drvdata(spi, epd);
        epaper_drv_data_p = epd;
    } else
        kfree(epd);
    return err;
}

static int epaper_spi_remove(struct spi_device *spi)
{
    struct epaper_drv_data *epd = spi_get_drvdata(spi);

    spin_lock_irq(&epd->spi_lock);
    epd->spi = NULL;
    epaper_drv_data_p = NULL;
    spin_unlock_irq(&epd->spi_lock);
    device_destroy(epaper_class, epd->devt);
    devm_gpiod_put(&spi->dev, gpio_to_desc(epd->reset_gpio));
    devm_gpiod_put(&spi->dev, gpio_to_desc(epd->dc_gpio));
    devm_gpiod_put(&spi->dev, gpio_to_desc(epd->busy_gpio));
    kfree(epd);

    return 0;
}

static struct spi_driver epaper_spi_driver = {
    .driver = {
        .name =         "epaper_spi",
        .owner =        THIS_MODULE,
        .of_match_table = of_match_ptr(epaper_dt_ids),
     },
    .probe =        epaper_spi_probe,
    .remove =       epaper_spi_remove,
};

static int __init epaper_spi_init(void)
{
    int status;
    status = register_chrdev(EPAPER_SPI_MAJOR, "epaper_spi", &epaper_fops);
    if(status < 0)
        return status;
    
    epaper_class = class_create(THIS_MODULE, "epaper_spi");
    if(IS_ERR(epaper_class)) {
        unregister_chrdev(EPAPER_SPI_MAJOR, epaper_spi_driver.driver.name);
        return PTR_ERR(epaper_class);
    }

    status = spi_register_driver(&epaper_spi_driver);
    if(status < 0) {
        class_destroy(epaper_class);
        unregister_chrdev(EPAPER_SPI_MAJOR, epaper_spi_driver.driver.name);
    }
    return status;
}
module_init(epaper_spi_init);

static void __exit epaper_spi_exit(void)
{
    spi_unregister_driver(&epaper_spi_driver);
    class_destroy(epaper_class);
    unregister_chrdev(EPAPER_SPI_MAJOR, epaper_spi_driver.driver.name);
}
module_exit(epaper_spi_exit);

MODULE_AUTHOR("YoRHa A2, <yorha.a2@foxmail.com");
MODULE_LICENSE("GPL v2");