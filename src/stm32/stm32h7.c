// Code to setup clocks and gpio on stm32h7 
//
// Copyright (C) 2020 Konstantin Vogel <konstantin.vogel@gmx.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.


// USB AND I2C NOT SUPPORTED!!

#include "autoconf.h" // CONFIG_CLOCK_REF_FREQ
#include "board/armcm_boot.h" // VectorTable
#include "board/irq.h" // irq_disable
#include "command.h" // DECL_CONSTANT_STR
#include "internal.h" // enable_pclock
#include "sched.h" // sched_main

#define FREQ_PERIPH (CONFIG_CLOCK_FREQ / 4)

// Enable a peripheral clock
void
enable_pclock(uint32_t periph_base)
{
    // periph_base determines in which bitfield at wich position to set a bit
    // E.g. D2_AHB1PERIPH_BASE is the adress offset of the given bitfield
    // the naming makes 0% sense
    if (periph_base < D2_APB2PERIPH_BASE) {
        uint32_t pos = (periph_base - D2_APB1PERIPH_BASE) / 0x400;
        RCC->APB1LENR |= (1<<pos);// we assume it is not in APB1HENR
        RCC->APB1LENR;
    } else if (periph_base < D2_AHB1PERIPH_BASE) {
        uint32_t pos = (periph_base - D2_APB2PERIPH_BASE) / 0x400;
        RCC->APB2ENR |= (1<<pos);
        RCC->APB2ENR;
    } else if (periph_base < D2_AHB2PERIPH_BASE) {
        uint32_t pos = (periph_base - D2_AHB1PERIPH_BASE) / 0x400;
        RCC->AHB1ENR |= (1<<pos);
        RCC->AHB1ENR;
    } else if (periph_base < D1_APB1PERIPH_BASE) {
        uint32_t pos = (periph_base - D2_AHB2PERIPH_BASE) / 0x400;
        RCC->AHB2ENR |= (1<<pos);
        RCC->AHB2ENR;
    } else if (periph_base < D1_AHB1PERIPH_BASE) {
        uint32_t pos = (periph_base - D1_APB1PERIPH_BASE) / 0x400;
        RCC->APB3ENR |= (1<<pos);
        RCC->APB3ENR;
    } else if (periph_base < D3_APB1PERIPH_BASE) {
        uint32_t pos = (periph_base - D1_AHB1PERIPH_BASE) / 0x400;
        RCC->AHB3ENR |= (1<<pos);
        RCC->AHB3ENR;
    } else if (periph_base < D3_AHB1PERIPH_BASE) {
        uint32_t pos = (periph_base - D3_APB1PERIPH_BASE) / 0x400;
        RCC->APB4ENR |= (1<<pos);
        RCC->APB4ENR;
    } else {
        uint32_t pos = (periph_base - D3_AHB1PERIPH_BASE) / 0x400;
        RCC->AHB4ENR |= (1<<pos);
        RCC->AHB4ENR;
    }
}

// Check if a peripheral clock has been enabled
int
is_enabled_pclock(uint32_t periph_base)
{
    if (periph_base < D2_APB2PERIPH_BASE) {
        uint32_t pos = (periph_base - D2_APB1PERIPH_BASE) / 0x400;
        return RCC->APB1LENR & (1<<pos);// we assume it is not in APB1HENR
    } else if (periph_base < D2_AHB1PERIPH_BASE) {
        uint32_t pos = (periph_base - D2_APB2PERIPH_BASE) / 0x400;
        return RCC->APB2ENR & (1<<pos);
    } else if (periph_base < D2_AHB2PERIPH_BASE) {
        uint32_t pos = (periph_base - D2_AHB1PERIPH_BASE) / 0x400;
        return RCC->AHB1ENR & (1<<pos);
    } else if (periph_base < D1_APB1PERIPH_BASE) {
        uint32_t pos = (periph_base - D2_AHB2PERIPH_BASE) / 0x400;
        return RCC->AHB2ENR & (1<<pos);
    } else if (periph_base < D1_AHB1PERIPH_BASE) {
        uint32_t pos = (periph_base - D1_APB1PERIPH_BASE) / 0x400;
        return RCC->APB3ENR & (1<<pos);
    } else if (periph_base < D3_APB1PERIPH_BASE) {
        uint32_t pos = (periph_base - D1_AHB1PERIPH_BASE) / 0x400;
        return RCC->AHB3ENR & (1<<pos);
    } else if (periph_base < D3_AHB1PERIPH_BASE) {
        uint32_t pos = (periph_base - D3_APB1PERIPH_BASE) / 0x400;
        return RCC->APB4ENR & (1<<pos);
    } else {
        uint32_t pos = (periph_base - D3_AHB1PERIPH_BASE) / 0x400;
        return RCC->AHB4ENR & (1<<pos);
    }
}

// Return the frequency of the given peripheral clock
uint32_t
get_pclock_frequency(uint32_t periph_base)
{
    return FREQ_PERIPH;
}

// Enable a GPIO peripheral clock
void
gpio_clock_enable(GPIO_TypeDef *regs)
{
    enable_pclock((uint32_t)regs);
}

// Set the mode and extended function of a pin TODO verify
void
gpio_peripheral(uint32_t gpio, uint32_t mode, int pullup)
{
    GPIO_TypeDef *regs = digital_regs[GPIO2PORT(gpio)];

    // Enable GPIO clock
    gpio_clock_enable(regs);

    // Configure GPIO
    uint32_t mode_bits = mode & 0xf, func = (mode >> 4) & 0xf, od = mode >> 8;
    uint32_t pup = pullup ? (pullup > 0 ? 1 : 2) : 0;
    uint32_t pos = gpio % 16, af_reg = pos / 8;
    uint32_t af_shift = (pos % 8) * 4, af_msk = 0x0f << af_shift;
    uint32_t m_shift = pos * 2, m_msk = 0x03 << m_shift;

    regs->AFR[af_reg] = (regs->AFR[af_reg] & ~af_msk) | (func << af_shift);
    regs->MODER = (regs->MODER & ~m_msk) | (mode_bits << m_shift);
    regs->PUPDR = (regs->PUPDR & ~m_msk) | (pup << m_shift);
    regs->OTYPER = (regs->OTYPER & ~(1 << pos)) | (od << pos);
    regs->OSPEEDR = (regs->OSPEEDR & ~m_msk) | (0x02 << m_shift);
}

void
wait()
{
    int i;
    for(i=0;i<50000;i++)
    {};
}

#if !CONFIG_STM32_CLOCK_REF_INTERNAL
DECL_CONSTANT_STR("RESERVE_PINS_crystal", "PH0,PH1");
#endif

// Main clock setup called at chip startup
static void
clock_setup(void)
{
    while (!(PWR->CSR1 & PWR_CSR1_ACTVOSRDY))
        ;
    uint32_t pll_base = 5000000; //  HSE(25mhz) /=DIVM1(5) -pll_base(5Mhz)-> *=DIVN1(192) -pll_freq-> /=DIVP1(2) -SYSCLK(480mhz)->
    uint32_t pll_freq = CONFIG_CLOCK_FREQ * 2; // only even dividers (DIVP1) are allowed
    if (!CONFIG_STM32_CLOCK_REF_INTERNAL) {
        // Configure PLL from external crystal (HSE)
        RCC->CR |= RCC_CR_HSEON; // enable HSE input
        wait();
        while(!(RCC->CR & RCC_CR_HSERDY))
            ;
        MODIFY_REG(RCC->PLLCKSELR, RCC_PLLCKSELR_PLLSRC_Msk, RCC_PLLCKSELR_PLLSRC_HSE); // choose HSE as clock source
        MODIFY_REG(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM1_Msk, (CONFIG_CLOCK_REF_FREQ/pll_base) << RCC_PLLCKSELR_DIVM1_Pos);// set pre divider DIVM1
    } else {
        // Configure PLL from internal 64Mhz oscillator (HSI)
        pll_base = 4000000; //64mhz is integer divisible with 4mhz
        MODIFY_REG(RCC->PLLCKSELR, RCC_PLLCKSELR_PLLSRC_Msk, RCC_PLLCKSELR_PLLSRC_HSI); // choose HSI as clock source
        MODIFY_REG(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM1_Msk, (64000000 / pll_base) << RCC_PLLCKSELR_DIVM1_Pos);// set pre divider DIVM1
    }
    MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLL1FRACEN, 0); // Default should already be 0
    MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLL1VCOSEL_Msk, 0); // Default should already be 0
    MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLL1RGE_Msk, RCC_PLLCFGR_PLL1RGE_2); // set frequency range of PLL1 according to pll_base (3=8-16Mhz, 2=4-8Mhz)
    MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_DIVR1EN_Msk, 0); // Disable unused outputs
    MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_DIVQ1EN_Msk, 0);
    MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_DIVP1EN_Msk, RCC_PLLCFGR_DIVP1EN); // This is necessary (default value in reference manual seems incorrect)

    MODIFY_REG(RCC->PLL1DIVR, RCC_PLL1DIVR_N1_Msk, ((pll_freq/pll_base)-1)          << RCC_PLL1DIVR_N1_Pos); // Set multiplier DIVN1
    MODIFY_REG(RCC->PLL1DIVR, RCC_PLL1DIVR_P1_Msk, ((pll_freq/CONFIG_CLOCK_FREQ)-1) << RCC_PLL1DIVR_P1_Pos); // Set post divider DIVP1 (here 001 = /2, 011 = not allowed, 0011 = /4...)

    // Crank up Vcore
    MODIFY_REG(PWR->D3CR, PWR_D3CR_VOS_Msk, PWR_D3CR_VOS);
    wait();
    while (!(PWR->D3CR & PWR_D3CR_VOSRDY))
        ;
    // Enable VOS0 (overdrive), only relevant for revision V or later @480mhz 
    RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
    SYSCFG->PWRCR |= SYSCFG_PWRCR_ODEN;
    wait();
    while (!(PWR->D3CR & PWR_D3CR_VOSRDY))
        ;

    // Set flash latency (pg.159)
    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY_Msk, FLASH_ACR_LATENCY_2WS);
    MODIFY_REG(FLASH->ACR, FLASH_ACR_WRHIGHFREQ_Msk, FLASH_ACR_WRHIGHFREQ_1);
    while (!(FLASH->ACR & FLASH_ACR_LATENCY_2WS))
        ;

    // Switch on PLL1
    RCC->CR |= RCC_CR_PLL1ON;
    wait();
    while (!(RCC->CR & RCC_CR_PLL1RDY))// Wait for PLL lock
        ;

    MODIFY_REG(RCC->D1CFGR, RCC_D1CFGR_HPRE_Msk,    RCC_D1CFGR_HPRE_DIV2);
    // Set D1PPRE, D2PPRE, D2PPRE2, D3PPRE 
    MODIFY_REG(RCC->D1CFGR, RCC_D1CFGR_D1PPRE_Msk,  RCC_D1CFGR_D1PPRE_DIV2);
    MODIFY_REG(RCC->D2CFGR, RCC_D2CFGR_D2PPRE1_Msk, RCC_D2CFGR_D2PPRE1_DIV2);
    MODIFY_REG(RCC->D2CFGR, RCC_D2CFGR_D2PPRE2_Msk, RCC_D2CFGR_D2PPRE2_DIV2);
    MODIFY_REG(RCC->D3CFGR, RCC_D3CFGR_D3PPRE_Msk,  RCC_D3CFGR_D3PPRE_DIV2);

    // Switch system clock source (SYSCLK) to PLL1
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW_Msk, RCC_CFGR_SW_PLL1);
    // Wait for PLL1 to be selected
    wait();
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL1)
        ;
}

// Main entry point - called from armcm_boot.c:ResetHandler()
void
armcm_main(void)
{
    // Run SystemInit() and then restore VTOR
    SystemInit();

    SCB->VTOR = (uint32_t)VectorTable;

    clock_setup();

    sched_main();
}
