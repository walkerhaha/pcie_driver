#include <linux/module.h>

MODULE_DESCRIPTION("MT EMU qy PCIe Test Linux Driver");

MODULE_AUTHOR("Yong Liu <yliu@mthreads.com>");
MODULE_LICENSE("GPL v2");

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/err.h>
#include <linux/aer.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/msi.h>
#include <linux/pci-epf.h>
#include <linux/miscdevice.h>
#include <linux/dmaengine.h>

#include "module_reg.h"
#include "mt-emu-drv.h"
#include "mt-emu.h"

#include "mt-emu-mtdma-bare.h"
#include <linux/random.h>

void mtdma_comm_init(void __iomem * mtdma_comm_vaddr, int vf_num) {
	int i;
	u64 dma_mask = 0x0000ffffffffffffULL;

	SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_COMM_ENABLE, BIT(0)); //osid_en
	SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_CH_NUM, PCIE_DMA_CH_NUM-1);
	SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_RCH_PF, ((DMA_WR_CH_DEPTH*PCIE_DMA_CH_NUM)/PCIE_DMA_CH_RD_NUM)-1);
	SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_WCH_PF, ((DMA_RD_CH_DEPTH*PCIE_DMA_CH_NUM)/PCIE_DMA_CH_WR_NUM)-1);
	SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_MST0_BLEN, (MST0_ARLEN<<4) | MST0_AWLEN);
	SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_MST1_BLEN, (MST1_ARLEN<<4) | MST1_AWLEN);
	dma_mask >>= vf_num;
	SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_COMM_ALARM_IMSK, 0);
	SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_RD_MRG_PF0_IMSK_L, ~dma_mask);
	SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_RD_MRG_PF0_IMSK_H, ~(dma_mask>>32));
	SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_WR_MRG_PF0_IMSK_L, ~dma_mask);
	SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_WR_MRG_PF0_IMSK_H, ~(dma_mask>>32));
	if(GET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_WORK_STS)) {
		printk( "dma init with busy error\n");
	}

	//	for(i=0; i<PCIE_DMA_CH_NUM; i++) {
	//		if(i<PCIE_DMA_CH_NUM-vf_num) {
	//			SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_CH_OSID(i), BIT(16)); //hyper pf0
	//		}
	//		else {
	//			SET_COMM_32(mtdma_comm_vaddr, REG_DMA_COMM_CH_OSID(i), i - (PCIE_DMA_CH_NUM - vf_num)); //vf
	//		}
	//	}
}

void build_dma_info(void *mtdma_vaddr, uint64_t mtdma_paddr, void __iomem *rg_vaddr, void __iomem *ll_vaddr, u8 vf, u8 wr_ch_cnt, u8 rd_ch_cnt, struct mtdma_info *dma_info)
{
	struct mtdma_chan_info *chan_info;
	off_t off,off_system;
	u32 ll_sz;
	int ch_cnt;
	int i, j;

	dma_info->wr_ch_cnt = wr_ch_cnt;
	dma_info->rd_ch_cnt = rd_ch_cnt;

	for(j =0; j<2; j++) {
		if (j == 0) {
			chan_info = dma_info->wr_ch_info;
			ch_cnt = dma_info->wr_ch_cnt;
			off = 0x80000000;
			off_system = 0;
		}
		else {
			chan_info = dma_info->rd_ch_info;
			ch_cnt = dma_info->rd_ch_cnt;
			off = 0x80000000+wr_ch_cnt*65536*32;
			off_system = wr_ch_cnt*65536*32;
		}

		ll_sz = 65536*32;

		for (i = 0; i < ch_cnt; i++) {

			chan_info[i].ll_max = 65536;
			chan_info[i].ll_laddr = ll_sz * i + off;
			chan_info[i].ll_vaddr = ll_vaddr + chan_info[i].ll_laddr;
			chan_info[i].ll_vaddr_system = mtdma_vaddr + off_system + ll_sz * i;
			chan_info[i].ll_laddr_system = 0x000000000000 + off_system + ll_sz * i;

			if(vf) {
				chan_info[i].rg_vaddr = rg_vaddr + (j == 0 ? 0x3000+0x800 : 0x3000);
			}
			else {
				chan_info[i].rg_vaddr = rg_vaddr + (j == 0 ? 0x383000 + 0x1000 * i + 0x800 : 0x383000+0x1000*i);
			}

			//pr_debug("chan_info vf(%d) wr(%d) ch %d: rg_vaddr=%px, ll_max=%x, ll_laddr=%llx, ll_vaddr=%px}\n", vf ? 1 : 0,
			//	j, i, chan_info[i].rg_vaddr, chan_info[i].ll_max, chan_info[i].ll_laddr, chan_info[i].ll_vaddr);
		}
	}
}

void mtdma_bare_init(struct dma_bare *dma_bare, struct mtdma_info *info) {
	int i;

	dma_bare->wr_ch_cnt = info->wr_ch_cnt;
	dma_bare->rd_ch_cnt = info->rd_ch_cnt;

	for(i = 0; i < info->wr_ch_cnt; i++) {
		struct dma_bare_ch   *chan = &dma_bare->wr_ch[i];

		chan->info = info->wr_ch_info[i];
		init_completion(&chan->int_done);
		mutex_init(&chan->int_mutex);

		if(GET_CH_32(chan, REG_DMA_CH_INTR_RAW)) {
			SET_CH_32(chan, REG_DMA_CH_INTR_RAW, GET_CH_32(chan, REG_DMA_CH_INTR_RAW));
		}
	}

	for(i=0; i<info->rd_ch_cnt; i++) {
		struct dma_bare_ch   *chan = &dma_bare->rd_ch[i];

		chan->info = info->rd_ch_info[i];
		init_completion(&chan->int_done);
		mutex_init(&chan->int_mutex);

		if(GET_CH_32(chan, REG_DMA_CH_INTR_RAW)) {
			SET_CH_32(chan, REG_DMA_CH_INTR_RAW, GET_CH_32(chan, REG_DMA_CH_INTR_RAW));
		}
	}
}

int dma_bare_isr(struct dma_bare_ch *bare_ch) {
	u32 val;
	val = GET_CH_32(bare_ch, REG_DMA_CH_INTR_RAW);

	if(val & DMA_CH_INTR_BIT_DONE) {
		bare_ch->int_error = 0;
	}
	else if(val & (DMA_CH_INTR_BIT_ERR_DATA | DMA_CH_INTR_BIT_ERR_DESC_READ | DMA_CH_INTR_BIT_ERR_CFG |DMA_CH_INTR_BIT_ERR_DUMMY_READ)) {
		bare_ch->int_error = 1;
		if(val & DMA_CH_INTR_BIT_ERR_DATA)
			printk("DMA int DATA error(%x)\n", val);
		if( val & DMA_CH_INTR_BIT_ERR_DESC_READ)
			printk("DMA int DESC_READ error(%x)\n", val);
		if(val & DMA_CH_INTR_BIT_ERR_CFG)
			printk("DMA int CFG error(%x)\n", val);
		if(val & DMA_CH_INTR_BIT_ERR_DUMMY_READ)
			printk("DMA int DUMMY_READ error(%x)\n", val);
	}
	else 
		printk("NONE ERROR ,num :%x\n", val);

	SET_CH_32(bare_ch, REG_DMA_CH_INTR_RAW, val);
	//Read to make sure pcie wr is completed.
	GET_CH_32(bare_ch, REG_DMA_CH_INTR_RAW);

	complete(&bare_ch->int_done);

	if(bare_ch->int_error)
		return -1;
	else
		return 0;
}

static void dma_desc_dump(struct dma_bare_ch *bare_ch, struct dma_ch_desc * *lli_rw, u32 lli_num){
	int i;

	for(i=0; i<lli_num+1; i++) {
		struct dma_ch_desc *lli;

		if(i==0) {
			lli = bare_ch->info.rg_vaddr + REG_DMA_CH_DESC_OPT;
		}
		else {
			lli = lli_rw[i-1];
		}

		printk("desc_op = %x\n", GET_LL_32(lli, desc_op));
		printk("cnt = %x\n", GET_LL_32(lli, cnt));
		printk("sar.l = %x\n", GET_LL_32(lli, sar.lsb));
		printk("sar.h = %x\n", GET_LL_32(lli, sar.msb));
		printk("dar.l = %x\n", GET_LL_32(lli, dar.lsb));
		printk("dar.h = %x\n", GET_LL_32(lli, dar.msb));
		printk("lar.l = %x\n", GET_LL_32(lli, lar.lsb));
		printk("lar.h = %x\n", GET_LL_32(lli, lar.msb));
	}

}

int dma_bare_xfer(struct dma_bare_ch *bare_ch, uint32_t data_direction, uint32_t desc_direction, uint32_t desc_cnt, uint32_t block_cnt, uint64_t sar, uint64_t dar, uint32_t size, uint32_t ch_num, uint32_t timeout_ms) {
	uint32_t ch_en = 0;
	uint32_t direction = 0;
	uint32_t elm_cnt;
	uint32_t chain_en;
	uint32_t size_tmp;
	uint32_t dummy_read_addr;
	uint32_t dummy_addr_L;
	uint32_t dummy_addr_H;
	uint64_t sar_tmp, dar_tmp;
	int i, j, ret;
	struct dma_ch_desc *lli;
	//struct dma_ch_desc *lli_rw[65536];
	uint32_t offset=0;
	uint32_t rand_flag;
	uint32_t desc_cnt_max;
	u32 desc_cnt_total=0;
	rand_flag = desc_cnt>>31;
	desc_cnt_max = desc_cnt & 0x7fffffff;

	chain_en = (desc_cnt==0) ? 0 : 1;
	size_tmp = size;

	sar_tmp = sar;
	dar_tmp = dar;

	//00:H2D(cross and no dummy read)  
	//01:H2H(no cross no dummy read) 
	//10:H2D(cross no dummy read) 
	//11:H2H(no cross and dummy read)

	//00:D2H(cross and no dummy read)  
	//01:D2D(no cross no dummy read) 
	//10:D2H(cross and dummy read) 
	//11:D2D(no cross and no dummy read)
	printk("dummy addr TEST\n");

	switch(data_direction) {
		case DMA_MEM_TO_MEM:
			direction |= DMA_CH_EN_BIT_NOCROSS;
			direction |= DMA_CH_EN_BIT_DUMMY;
			printk("DMA_MEM_TO_MEM direction %d\n",direction);	
			break;
		case DMA_MEM_TO_DEV:
			printk("DMA_MEM_TO_DEV direction %d\n",direction);	
			break;
		case DMA_DEV_TO_MEM:
			direction |= DMA_CH_EN_BIT_DUMMY;
			printk("DMA_DEV_TO_MEM direction %d\n",direction);	
			break;
		case DMA_DEV_TO_DEV:
			direction |= DMA_CH_EN_BIT_NOCROSS;
			printk("DMA_DEV_TO_DEV direction %d\n",direction);	
			break;
		default:
			return -1;
	}


	if((desc_direction==DMA_DESC_IN_DEVICE)&&((data_direction==DMA_MEM_TO_DEV)||(data_direction==DMA_MEM_TO_MEM)))
		direction |= DMA_CH_EN_BIT_DESC_MST1;
	else if((desc_direction!=DMA_DESC_IN_DEVICE)&&((data_direction==DMA_DEV_TO_DEV)||(data_direction==DMA_DEV_TO_MEM)))
		direction |= DMA_CH_EN_BIT_DESC_MST1;


	ch_en |= DMA_CH_EN_BIT_ENABLE;

	mutex_lock(&bare_ch->int_mutex);
	reinit_completion(&bare_ch->int_done);
	bare_ch->int_error = 0x0;

	if(desc_cnt==0) {
		u32 cnt = size;

		if(i==0) {
			lli = bare_ch->info.rg_vaddr + REG_DMA_CH_DESC_OPT;
			u32 ch_lbar_basic = (desc_cnt<<16) | chain_en;
			SET_CH_32(bare_ch, REG_DMA_CH_LBAR_BASIC,ch_lbar_basic);
		}
#if (MTDMA_MMU==1)
		SET_CH_32(bare_ch, REG_DMA_CH_MMU_ADDR_TYPE, 0x101);
#endif
		SET_LL_32(lli, cnt, cnt-1);
		SET_LL_32(lli, sar.lsb, lower_32_bits(sar));
		SET_LL_32(lli, sar.msb, upper_32_bits(sar));
		SET_LL_32(lli, dar.lsb, lower_32_bits(dar));
		SET_LL_32(lli, dar.msb, upper_32_bits(dar));

		if( direction > 3) {
			dummy_addr_H = GET_CH_32(bare_ch, REG_DUMMY_CH_ADDR_H);
			dummy_addr_L = GET_CH_32(bare_ch, REG_DUMMY_CH_ADDR_L);	

			printk("dummy addr H:%x\n",dummy_addr_H);
			printk("dummy addr L:%x\n",dummy_addr_L);
		}

	}

	/*if((data_direction==DMA_DEV_TO_MEM)||(data_direction==DMA_MEM_TO_MEM)) {
	  dummy_addr_H = GET_CH_32(bare_ch, REG_DUMMY_CH_ADDR_H);
	  dummy_addr_L = GET_CH_32(bare_ch, REG_DUMMY_CH_ADDR_L);	

	  printk("dummy addr H%x\n",dummy_addr_H);
	  printk("dummy addr L%x\n",dummy_addr_L);
	  }*/

	/////////////////--------chain task-------//////////////////////
	else {
		if(block_cnt==0) {
			u32 desc_cnt_tmp;
			void* desc_size;
			desc_size = kmalloc(100000, GFP_KERNEL);
			if(desc_size==NULL)
				printk("desc_size kmalloc failed\n");
			if(rand_flag==1) {
				size_tmp = size;
				i = 0;
				while(size_tmp!=0) {
					unsigned long r;
					get_random_bytes(&r, sizeof(unsigned long));
					u32 size_rand = r%(64*1024-4*1024) + 4*1024;
					//u32 size_rand = r%(4*1024) + 4*1024;
					//printk("size_rand=%d\n",size_rand);
					((u32*)desc_size)[i]=(size_rand>size_tmp) ? size_tmp : size_rand;
					//printk("desc_size %d = %d\n", i, ((u32*)desc_size)[i]);
					size_tmp -= ((u32*)desc_size)[i];
					desc_cnt_tmp = i;
					i++;
				}
				//	printk("desc_cnt_tmp=%u\n",desc_cnt_tmp);
			}
			else {
				desc_cnt_tmp = desc_cnt;
				size_tmp = size;
				elm_cnt = size_tmp/(desc_cnt_tmp+1);
			}

			sar_tmp = sar;
			dar_tmp = dar;

			for(i=0; i<=desc_cnt_tmp; i++) {
				u64 lar;
				u32 cnt;
				if(rand_flag==1) {
					elm_cnt = ((u32*)desc_size)[i];
					cnt = elm_cnt;
				} 
				else {
					cnt = (i==desc_cnt_tmp) ? size_tmp : elm_cnt;
				}

				if(i==0) {
#if (MTDMA_MMU==1)
					SET_CH_32(bare_ch, REG_DMA_CH_MMU_ADDR_TYPE, 0x101);
#endif
					lli = bare_ch->info.rg_vaddr + REG_DMA_CH_DESC_OPT;
					u32 ch_lbar_basic = (desc_cnt_tmp<<16) | chain_en;
					SET_CH_32(bare_ch, REG_DMA_CH_LBAR_BASIC,ch_lbar_basic);
				}
				else {
					if(desc_direction==DMA_DESC_IN_DEVICE)
						lli = &(((struct dma_ch_desc *)bare_ch->info.ll_vaddr)[i-1]);
					else
						lli = (struct dma_ch_desc *)(bare_ch->info.ll_vaddr_system + (i-1) * sizeof(struct dma_ch_desc));
					//lli_rw[i-1] = lli;
				}

				if(desc_direction==DMA_DESC_IN_DEVICE)
					lar = bare_ch->info.ll_laddr + (i * sizeof(struct dma_ch_desc));
				else
					lar = bare_ch->info.ll_laddr_system + i * sizeof(struct dma_ch_desc);

				SET_LL_32(lli, desc_op, 0);
				SET_LL_32(lli, cnt, cnt-1);
				SET_LL_32(lli, sar.lsb, lower_32_bits(sar_tmp));
				SET_LL_32(lli, sar.msb, upper_32_bits(sar_tmp));
				SET_LL_32(lli, dar.lsb, lower_32_bits(dar_tmp));
				SET_LL_32(lli, dar.msb, upper_32_bits(dar_tmp));
				SET_LL_32(lli, lar.lsb, lower_32_bits(lar));
				SET_LL_32(lli, lar.msb, upper_32_bits(lar));

				GET_LL_32(lli, desc_op);
				GET_LL_32(lli, cnt);
				GET_LL_32(lli, sar.lsb);
				GET_LL_32(lli, sar.msb);
				GET_LL_32(lli, dar.lsb);
				GET_LL_32(lli, dar.msb);
				GET_LL_32(lli, lar.lsb);
				GET_LL_32(lli, lar.msb);

				sar_tmp += elm_cnt;
				dar_tmp += elm_cnt;
				size_tmp -= cnt;

			}

			if( direction > 3) {
				dummy_addr_H = GET_CH_32(bare_ch, REG_DUMMY_CH_ADDR_H);
				dummy_addr_L = GET_CH_32(bare_ch, REG_DUMMY_CH_ADDR_L);	

				printk("dummy addr H:%x\n",dummy_addr_H);
				printk("dummy addr L:%x\n",dummy_addr_L);
			}
			kfree(desc_size);
		} 

		/////////////////--------block type-------//////////////////////
		else {
			for(j=0; j<block_cnt; j++) {
				u32 desc_cnt_tmp;
				void* desc_size;
				desc_size = kmalloc(100000, GFP_KERNEL);
				if(desc_size==NULL)
					printk("desc_size kmalloc failed\n");
				if(rand_flag==1) {
					size_tmp = size;
					i = 0;
					while(size_tmp!=0) {
						unsigned long r;
						get_random_bytes(&r, sizeof(unsigned long));
						u32 size_rand = r%(64*1024-4*1024) + 4*1024;
						//u32 size_rand = r%(4*1024) + 4*1024;
						//printk("size_rand=%d\n",size_rand);
						((u32*)desc_size)[i]=(size_rand>size_tmp) ? size_tmp : size_rand;
						//printk("desc_size %d = %d\n",i,((u32*)desc_size)[i]);
						size_tmp -= ((u32*)desc_size)[i];
						desc_cnt_tmp = i;
						i++;
					}
					if(j!=0)
						desc_cnt_tmp++;
					printk("desc_cnt_tmp=%u\n",desc_cnt_tmp);
				}
				else {
					desc_cnt_tmp = desc_cnt;
					size_tmp = size;
				}
				///////////////------frist bit--------------////////////////
				if(j==0) {
					elm_cnt = size_tmp/(desc_cnt_tmp+1);
					sar_tmp = sar;
					dar_tmp = dar;

					for(i=0; i<=desc_cnt_tmp; i++) {
						u64 lar;
						u32 cnt;

						if(rand_flag==1) {
							elm_cnt = ((u32*)desc_size)[i];
							cnt = elm_cnt;
						}
						else {
							cnt = (i==desc_cnt_tmp) ? size_tmp : elm_cnt;
						}

						if(i==0) {
							lli = bare_ch->info.rg_vaddr + REG_DMA_CH_DESC_OPT;
							u32 ch_lbar_basic = (desc_cnt_tmp<<16) | chain_en;
							SET_CH_32(bare_ch, REG_DMA_CH_LBAR_BASIC,ch_lbar_basic);
						}
						else {
							if(desc_direction==DMA_DESC_IN_DEVICE)
								lli = &(((struct dma_ch_desc *)bare_ch->info.ll_vaddr)[i-1]);
							else
								lli = (struct dma_ch_desc *)(bare_ch->info.ll_vaddr_system + (i-1) * sizeof(struct dma_ch_desc));
							//lli_rw[i-1] = lli;
						}

						if(desc_direction==DMA_DESC_IN_DEVICE)
							lar = bare_ch->info.ll_laddr + (i * sizeof(struct dma_ch_desc)) + offset*32;
						else
							lar = bare_ch->info.ll_laddr_system + i * sizeof(struct dma_ch_desc) + offset*32;

						//if((i==desc_cnt_tmp)&&(lar%64!=0)){
						if(i==desc_cnt_tmp){
							//offset += 1;
							//printk("offset=%d,lar=%lx\n",offset,lar);
							offset += 32-(lar%1024)/32;
							lar += 1024-(lar%1024);
						}
						if((i==desc_cnt_tmp)&&(j!=block_cnt-1)) {
							SET_LL_32(lli, desc_op, (desc_cnt_tmp*65536 + 1));
							printk("block%d-desc%d-dword0=0x%x\n",j,i,(desc_cnt_tmp*65536 + 1));
						} else {
							SET_LL_32(lli, desc_op, 0);
							printk("block%d-desc%d-dword0=0x%x\n",j,i,0);
						}

						printk("block%d-desc%d-dword1=0x%x\n",j,i,cnt-1);
						printk("block%d-desc%d-dword2=0x%x\n",j,i,lower_32_bits(sar_tmp));
						printk("block%d-desc%d-dword3=0x%x\n",j,i,upper_32_bits(sar_tmp));
						printk("block%d-desc%d-dword4=0x%x\n",j,i,lower_32_bits(dar_tmp));
						printk("block%d-desc%d-dword5=0x%x\n",j,i,upper_32_bits(dar_tmp));
						printk("block%d-desc%d-dword6=0x%x\n",j,i,lower_32_bits(lar));
						printk("block%d-desc%d-dword7=0x%x\n",j,i,upper_32_bits(lar));

						SET_LL_32(lli, cnt, cnt-1);
						SET_LL_32(lli, sar.lsb, lower_32_bits(sar_tmp));
						SET_LL_32(lli, sar.msb, upper_32_bits(sar_tmp));
						SET_LL_32(lli, dar.lsb, lower_32_bits(dar_tmp));
						SET_LL_32(lli, dar.msb, upper_32_bits(dar_tmp));
						SET_LL_32(lli, lar.lsb, lower_32_bits(lar));
						SET_LL_32(lli, lar.msb, upper_32_bits(lar));

						GET_LL_32(lli, cnt);
						GET_LL_32(lli, sar.lsb);
						GET_LL_32(lli, sar.msb);
						GET_LL_32(lli, dar.lsb);
						GET_LL_32(lli, dar.msb);
						GET_LL_32(lli, lar.lsb);
						u32 dbg = GET_LL_32(lli, lar.msb);

						//printk("block=%d,desc=%d,cnt=%d\n",j,i,cnt);	
					}
					}

					else {
						elm_cnt = size_tmp/desc_cnt_tmp;
						sar_tmp = sar+j*size;
						dar_tmp = dar+j*size;

						for(i=0; i<desc_cnt_tmp; i++) {
							u64 lar;
							u32 cnt;

							if(rand_flag==1) {
								elm_cnt = ((u32*)desc_size)[i];
								cnt = elm_cnt;
							}else{
								cnt = (i==(desc_cnt_tmp-1)) ? size_tmp : elm_cnt;
							}

							if(desc_direction==DMA_DESC_IN_DEVICE) {
								if(rand_flag==1) {
									lli = &(((struct dma_ch_desc *)bare_ch->info.ll_vaddr)[i+desc_cnt_total+offset]);
								}
								else {
									lli = &(((struct dma_ch_desc *)bare_ch->info.ll_vaddr)[i+j*desc_cnt_tmp+offset]);
								}
							}
							else
								lli = (struct dma_ch_desc *)(bare_ch->info.ll_vaddr_system + (i+j*desc_cnt_tmp+offset) * sizeof(struct dma_ch_desc));

							if(desc_direction==DMA_DESC_IN_DEVICE) {
								if(rand_flag==1) {
									lar = bare_ch->info.ll_laddr + ((i+desc_cnt_total+1) * sizeof(struct dma_ch_desc)) + offset*32;
								}
								else {
									lar = bare_ch->info.ll_laddr + ((i+j*desc_cnt_tmp+1) * sizeof(struct dma_ch_desc)) + offset*32;
								}
							}
							else
								lar = bare_ch->info.ll_laddr_system + (i+j*desc_cnt_tmp+1) * sizeof(struct dma_ch_desc) + offset*32;

							//if((i==(desc_cnt_tmp-1))&&(lar%64!=0)){
							//        offset += 1;
							//        lar += 32;
							//	//printk("offset=%d,lar=%lx\n",offset,lar);
							//}
							if(i==(desc_cnt_tmp-1)) {
								//offset += 1;
								//printk("offset=%d,lar=%lx\n",offset,lar);
								offset += 32-(lar%1024)/32;
								lar += 1024-(lar%1024);
							}

							if((i==desc_cnt_tmp-1)&&(j!=block_cnt-1)) {
								SET_LL_32(lli, desc_op, (desc_cnt_tmp*65536 + 1));
								printk("block%d-desc%d-dword0=0x%x\n",j,i,(desc_cnt_tmp*65536 + 1));
							} 
							else {
								SET_LL_32(lli, desc_op, 0);
								printk("block%d-desc%d-dword0=0x%x\n",j,i,0);
							}

							printk("block%d-desc%d-dword1=0x%x\n",j,i,cnt-1);
							printk("block%d-desc%d-dword2=0x%x\n",j,i,lower_32_bits(sar_tmp));
							printk("block%d-desc%d-dword3=0x%x\n",j,i,upper_32_bits(sar_tmp));
							printk("block%d-desc%d-dword4=0x%x\n",j,i,lower_32_bits(dar_tmp));
							printk("block%d-desc%d-dword5=0x%x\n",j,i,upper_32_bits(dar_tmp));
							printk("block%d-desc%d-dword6=0x%x\n",j,i,lower_32_bits(lar));
							printk("block%d-desc%d-dword7=0x%x\n",j,i,upper_32_bits(lar));

							SET_LL_32(lli, cnt, cnt-1);
							SET_LL_32(lli, sar.lsb, lower_32_bits(sar_tmp));
							SET_LL_32(lli, sar.msb, upper_32_bits(sar_tmp));
							SET_LL_32(lli, dar.lsb, lower_32_bits(dar_tmp));
							SET_LL_32(lli, dar.msb, upper_32_bits(dar_tmp));
							SET_LL_32(lli, lar.lsb, lower_32_bits(lar));
							SET_LL_32(lli, lar.msb, upper_32_bits(lar));
							SET_LL_32(lli, cnt, cnt-1);

							GET_LL_32(lli, sar.msb);
							GET_LL_32(lli, dar.lsb);
							GET_LL_32(lli, dar.msb);
							GET_LL_32(lli, lar.lsb);
							GET_LL_32(lli, lar.msb);

							printk("block=%d,desc=%d,cnt=%d\n",j,i,cnt);
							sar_tmp += elm_cnt;
							dar_tmp += elm_cnt;
							size_tmp -= cnt;
						}
					}
					desc_cnt_total += desc_cnt_tmp;
					printk("desc_cnt_total=%d\n", desc_cnt_total);
					if( direction > 3) {
						dummy_addr_H = GET_CH_32(bare_ch, REG_DUMMY_CH_ADDR_H);
						dummy_addr_L = GET_CH_32(bare_ch, REG_DUMMY_CH_ADDR_L);	

						printk("dummy addr H:%x\n",dummy_addr_H);
						printk("dummy addr L:%x\n",dummy_addr_L);
					}				
					kfree(desc_size);
				}
			}
		}
		int int_mask = 0;
		SET_CH_32(bare_ch, REG_DMA_CH_INTR_IMSK, 0);
		SET_CH_32(bare_ch, REG_DMA_CH_DIRECTION, direction);
		SET_CH_32(bare_ch, REG_DMA_CH_ENABLE, ch_en);	
		
		int_mask = GET_CH_32(bare_ch, REG_DMA_CH_INTR_IMSK);
		printk("xfer ch num: %d, int_mask :0x%x\n", bare_ch, int_mask);
		printk("print struct thing \n");
		printk(KERN_INFO "emu_pcie information:\n");
		printk(KERN_INFO "1: %x\n", bare_ch->info);
		printk(KERN_INFO "2: %x\n", bare_ch->int_done);
		printk(KERN_INFO "3: %x\n", bare_ch->int_mutex);
		printk(KERN_INFO "4: %x\n", bare_ch->int_error);
		printk("mtdma wait int\n");

		ret = wait_for_completion_timeout(&bare_ch->int_done, msecs_to_jiffies(timeout_ms));
		printk("xfer1 ch num: %d\n", bare_ch);	
		if(!ret) {
			pr_debug("wait dma int timeout%d\n");
			//dma_desc_dump(bare_ch, lli_rw, desc_cnt);
		}
		mutex_unlock(&bare_ch->int_mutex);
		if(ret) {
			if(bare_ch->int_error)
				return 2;
			return 1;
		}
		else
			return 0;
	}

