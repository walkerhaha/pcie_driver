#ifndef __QY_PLIC_F_H__
#define __QY_PLIC_F_H__

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

__attribute__((always_inline)) static inline void __nds__plic_set_feature (unsigned int base, unsigned int feature)
{
  pcief_sreg_u32(F_GPU, 0, base, feature);
}

__attribute__((always_inline)) static inline void __nds__plic_set_threshold (unsigned int base, unsigned int threshold, unsigned int target)
{
    pcief_sreg_u32(F_GPU, 0, base + PLIC_THRESHOLD_OFFSET + (target << PLIC_THRESHOLD_SHIFT_PER_TARGET), threshold);
}

__attribute__((always_inline)) static inline void __nds__plic_set_priority (unsigned int base, unsigned int source, unsigned int priority)
{
  pcief_sreg_u32(F_GPU, 0, base + PLIC_PRIORITY_OFFSET + (source << PLIC_PRIORITY_SHIFT_PER_SOURCE), priority);
}

__attribute__((always_inline)) static inline void __nds__plic_set_pending (unsigned int base, unsigned int source)
{
  pcief_sreg_u32(F_GPU, 0, base + PLIC_PENDING_OFFSET + ((source >> 5) << 2), 1 << (source & 0x1F));
}

__attribute__((always_inline)) static inline void __nds__plic_enable_interrupt (unsigned int base, unsigned int source, unsigned int target)
{
  unsigned int value = pcief_greg_u32(F_GPU, 0, base + PLIC_ENABLE_OFFSET + (target << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2));
  value = value | (1 << (source & 0x1F));
  pcief_sreg_u32(F_GPU, 0, base + PLIC_ENABLE_OFFSET + (target << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2), value);
}

__attribute__((always_inline)) static inline void __nds__plic_disable_interrupt (unsigned int base, unsigned int source, unsigned int target)
{
  unsigned int value = pcief_greg_u32(F_GPU, 0, base + PLIC_ENABLE_OFFSET + (target << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2));
  value = value & ~((1 << (source & 0x1F)));
  pcief_sreg_u32(F_GPU, 0, base + PLIC_ENABLE_OFFSET + (target << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2), value);
}

__attribute__((always_inline)) static inline unsigned int __nds__plic_claim_interrupt(unsigned int base, unsigned int target)
{
  return pcief_greg_u32(F_GPU, 0, base + PLIC_CLAIM_OFFSET + (target << PLIC_CLAIM_SHIFT_PER_TARGET));
}

__attribute__((always_inline)) static inline void __nds__plic_complete_interrupt(unsigned int base, unsigned int source, unsigned int target)
{
  pcief_sreg_u32(F_GPU, 0, base + PLIC_CLAIM_OFFSET + (target << PLIC_CLAIM_SHIFT_PER_TARGET), source);
}

#ifdef __cplusplus
}
#endif


#endif