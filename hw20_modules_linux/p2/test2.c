#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/rwlock.h>
#include <linux/uaccess.h>

static int major = 0;
static rwlock_t lock;
static char test_string[16] = "Hello, World!\n";

static loff_t test_llseek(struct file *file, loff_t off, int whence)
{
    return fixed_size_llseek(file, off, whence, 15);
}

static int test_open(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t test_read(struct file *file, char __user *buff, size_t size, loff_t *off)
{
    int len = strlen(test_string);
    
    if (*off >= len)
        return 0;
        
    if (size > len - *off)
        size = len - *off;
        
    read_lock(&lock);
    if (copy_to_user(buff, test_string + *off, size)) {
        read_unlock(&lock);
        return -EFAULT;
    }
    read_unlock(&lock);
    
    *off += size;
    return size;
}

static ssize_t test_write(struct file *file, const char __user *buff, size_t size, loff_t *off)
{
    if (size > 15)
        return -EINVAL;
        
    write_lock(&lock);
    if (copy_from_user(test_string, buff, size)) {
        write_unlock(&lock);
        return -EFAULT;
    }
    test_string[size] = '\0';
    write_unlock(&lock);
    
    return size;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .llseek = test_llseek,
    .open  = test_open,
    .read  = test_read,
    .write = test_write
};

static int __init test_init(void)
{
    pr_info("Test module loading...\n");
    rwlock_init(&lock);
    major = register_chrdev(0, "test2", &fops);
    if (major < 0) {
        pr_err("register_chrdev failed\n");
        return major;
    }
    pr_info("Major number: %d\n", major);
    return 0;
}

static void __exit test_exit(void)
{
    unregister_chrdev(major, "test2");
    pr_info("Test module unloaded\n");
}

module_init(test_init);
module_exit(test_exit);
MODULE_LICENSE("GPL");

