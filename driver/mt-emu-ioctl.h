#ifndef __PCIE_EMU_IOCTL__
#define __PCIE_EMU_IOCTL__





int mt_test_open(struct inode *inode, struct file *file);
ssize_t mt_test_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t mt_test_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
int mt_test_release(struct inode *inode, struct file *file);
int mt_test_mmap(struct file *file, struct vm_area_struct *vma);
long mt_test_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif
