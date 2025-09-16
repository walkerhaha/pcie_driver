#ifndef __QY_PLIC_H__
#define __QY_PLIC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* PLIC Feature Flags */
#define NDS_PLIC_FEATURE_PREEMPT        (1 << 0)
#define NDS_PLIC_FEATURE_VECTORED       (1 << 1)

/* Priority Register - 32 bits per source */
#define PLIC_PRIORITY_OFFSET            (0x00000000UL)
#define PLIC_PRIORITY_SHIFT_PER_SOURCE  2

/* Pending Register - 1 bit per soirce */
#define PLIC_PENDING_OFFSET             (0x00001000UL)
#define PLIC_PENDING_SHIFT_PER_SOURCE   0

/* Enable Register - 0x80 per target */
#define PLIC_ENABLE_OFFSET              (0x00002000UL)
#define PLIC_ENABLE_SHIFT_PER_TARGET    7

/* Priority Threshold Register - 0x1000 per target */
#define PLIC_THRESHOLD_OFFSET           (0x00200000UL)
#define PLIC_THRESHOLD_SHIFT_PER_TARGET 12

/* Claim Register - 0x1000 per target */
#define PLIC_CLAIM_OFFSET               (0x00200004UL)
#define PLIC_CLAIM_SHIFT_PER_TARGET     12

#define REG_BASE_PCIe_INTC	            (0x400000UL)
static inline void __nds__plic_set_feature (void __iomem *base, unsigned int feature)
{
  writel(feature, base + REG_BASE_PCIe_INTC);
}

static inline void __nds__plic_set_threshold (void __iomem *base, unsigned int threshold, unsigned int target)
{
    writel(threshold, base + REG_BASE_PCIe_INTC + PLIC_THRESHOLD_OFFSET + (target << PLIC_THRESHOLD_SHIFT_PER_TARGET));
}

static inline void __nds__plic_set_priority (void __iomem *base, unsigned int source, unsigned int priority)
{
  writel(priority, base + REG_BASE_PCIe_INTC + PLIC_PRIORITY_OFFSET + (source << PLIC_PRIORITY_SHIFT_PER_SOURCE));
}

static inline void __nds__plic_set_pending (void __iomem *base, unsigned int source)
{
  writel(1 << (source & 0x1F), base + REG_BASE_PCIe_INTC + PLIC_PENDING_OFFSET + ((source >> 5) << 2));
}

static inline void __nds__plic_enable_interrupt (void __iomem *base, unsigned int source, unsigned int target)
{
  u32 value = readl(base + REG_BASE_PCIe_INTC + PLIC_ENABLE_OFFSET + (target << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2));
  value = value | (1 << (source & 0x1F));
  writel(value, base + REG_BASE_PCIe_INTC + PLIC_ENABLE_OFFSET + (target << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2));
}

static inline void __nds__plic_disable_interrupt (void __iomem *base, unsigned int source, unsigned int target)
{
  u32 value = readl(base + REG_BASE_PCIe_INTC + PLIC_ENABLE_OFFSET + (target << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2));
  value = value & ~((1 << (source & 0x1F)));
  writel(value, base + REG_BASE_PCIe_INTC + PLIC_ENABLE_OFFSET + (target << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2));
}

static inline u32 __nds__plic_claim_interrupt(void __iomem *base, unsigned int target)
{
  return readl(base + REG_BASE_PCIe_INTC + PLIC_CLAIM_OFFSET + (target << PLIC_CLAIM_SHIFT_PER_TARGET));
}

static inline void __nds__plic_complete_interrupt(void __iomem *base, unsigned int source, unsigned int target)
{
  writel(source, base + REG_BASE_PCIe_INTC + PLIC_CLAIM_OFFSET + (target << PLIC_CLAIM_SHIFT_PER_TARGET));
}

#ifdef __cplusplus
}
#endif


#endif