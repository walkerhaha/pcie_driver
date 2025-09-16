// Copyright @2021 Moore Threads. All rights reserved.


#ifndef __INTD_H__
#define __INTD_H__

#include "qy_intd_def.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void intd_ctrl(void __iomem *base, uint32_t glb_enable) {
    writel(glb_enable, base + REG_INTD_CTRL);
}

static inline uint32_t intd_rd_sgi_sts(void __iomem *base) {
    return readl(base + REG_INTD_SGIRAW);
}

static inline uint32_t intd_rd_spi_sts(void __iomem *base, int i) {
    return readl(base + REG_INTD_SPIRAW(i));
}

static inline uint32_t intd_rd_target_sts(void __iomem *base, int i, int j) {
    return readl(base + REG_INTD_TST(i, j));
}

static inline uint32_t intd_rd_error_sts(void __iomem *base, int i) {
    return readl(base + REG_INTD_ERRIN(i));
}

static inline uint32_t intd_rd_target_error_sts(void __iomem *base) {
    return readl(base + REG_INTD_ERRC) >> 8;
}

static inline void intd_en_target_error(void __iomem *base, uint8_t targets) {
    writel(targets, base + REG_INTD_ERRC);
}

static inline void intd_set_sgi(void __iomem *base, int i, uint8_t lock, uint8_t key_p, uint8_t key, uint8_t plc, uint8_t target, uint8_t event, uint8_t en) {
    uint32_t val = (lock << 31) | (key_p<<30) | (key << 24) |(plc<<16) | (target<<8) | (event<<1) | (en);
    writel(val, base + REG_INTD_SGIC(i));
    val = readl(base + REG_INTD_SGIC(i));
}

/*
static inline void intd_smc_set_sgi_simple(int i, uint8_t target, uint8_t en) {
    intd_set_sgi(i, 0, 0, 0, INTD_PLC_SMC, target, 0, en);
}

static inline void intd_fec_set_sgi_simple(int i, uint8_t target, uint8_t en) {
    intd_set_sgi(i, 0, 0, 0, INTD_PLC_FEC, target, 0, en);
}*/

static inline void intd_pcie_set_sgi_simple(void __iomem *base, int i, uint8_t target, uint8_t en) {
    intd_set_sgi(base, i, 0, 0, 0, INTD_PLC_PCIE, target, 0, en);
}

static inline void intd_set_spi(void __iomem *base, int i, uint8_t lock, uint8_t key_p, uint8_t key, uint8_t plc, uint8_t target, uint8_t en) {
    uint32_t val = (lock << 31) | (key_p<<30) | (key << 24) |(plc<<16) | (target<<8) | (en);
    writel(val, base + REG_INTD_SPIC(i));
}

/*
static inline void intd_smc_set_spi_simple(int i, uint8_t target, uint8_t en) {
    intd_set_spi(i, 0, 0, 0, INTD_PLC_SMC, target, en);
}

static inline void intd_fec_set_spi_simple(int i, uint8_t target, uint8_t en) {
    intd_set_spi(i, 0, 0, 0, INTD_PLC_FEC, target, en);
}*/

static inline void intd_pcie_set_spi_simple(void __iomem *base, int i, uint8_t target, uint8_t en) {
    intd_set_spi(base, i, 0, 0, 0, INTD_PLC_PCIE, target, en);
}



#ifdef __cplusplus
}
#endif


#endif
