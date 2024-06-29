#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROCFS_NAME "usrpmc"

static struct proc_dir_entry *proc_file;

static ssize_t procfile_read( struct file *file, char __user *buffer, size_t len, loff_t *offset )
{
    unsigned long cr4;
    char output[20];
    int outputLen;

    asm volatile ( "mov %%cr4, %0" : "=r" ( cr4 ) );

    outputLen = snprintf( output, sizeof( output ), "0x%lx #CR4\n", cr4 );
    printk( KERN_INFO "/proc/%s read CR4 on processor %d\n", PROCFS_NAME, smp_processor_id() );

    return simple_read_from_buffer( buffer, len, offset, output, outputLen );
}

static ssize_t procfile_write( struct file *file, const char __user *buffer, size_t len, loff_t *offset )
{
    unsigned long cr4PmcInv;
    char input[2];

    if( len != 2 )
    {
        printk( KERN_INFO "/proc/%s incorrect input\n", PROCFS_NAME );
        return -EINVAL;
    }

    if( copy_from_user( &input, buffer, 2 ) )
    {
        printk( KERN_INFO "/proc/%s copy_from_user failed\n", PROCFS_NAME );
        return -EFAULT;
    }

    if( ( input[0]!='0' && input[0]!='1' ) || input[1]!='\n' )
    {
        printk( KERN_INFO "/proc/%s incorrect input\n", PROCFS_NAME );
        return -EINVAL;
    }

    cr4PmcInv = ( input[0]=='1' ) ? ( 1 << 8 ) : 0;

    printk( KERN_INFO "/proc/%s %s user space access to RDPMC on processor %d\n", PROCFS_NAME, ( input[0]=='0' ) ? "disable" : "enable", smp_processor_id() );

    asm volatile (
        "mov %%cr4, %%rax\n\t"   // Move CR4 register value to %1
        "and $0xfffffffffffffeff, %%rax\n\t"     // Set the 8th bit (1 << 8) in %1
        "or %0, %%rax\n\t"      // XOR the result with the initial value in %0
        "mov %%rax, %0\n\t"
        "mov %%rax, %%cr4"       // Move the modified value back to CR4
        : "+r" (cr4PmcInv)
        :
        : "rax"
    );

    return len;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = procfile_read,
    .proc_write = procfile_write,
};

static int __init usrpmcInit( void )
{
    proc_file = proc_create( PROCFS_NAME, 0666, NULL, &proc_file_ops );
    if( !proc_file )
    {
        printk( KERN_ALERT "Error: Could not initialize /proc/%s\n", PROCFS_NAME );
        return -ENOMEM;
    }
    printk( KERN_INFO "/proc/%s created\n", PROCFS_NAME );

    return 0;
}

static void __exit usrpmcExit( void )
{
    proc_remove( proc_file );
    printk( KERN_INFO "/proc/%s removed\n", PROCFS_NAME );
}

module_init( usrpmcInit );
module_exit( usrpmcExit );

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "Mikhail Gorbunov" );
MODULE_DESCRIPTION( "A kernel module to enable or disable rdpmc access from user space" );

