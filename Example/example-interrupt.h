/**
 * @file example-interrupt.h
 * @brief 例程2：DRDY 硬件外部中断采集 (EXTI1 中断)
 */

#ifndef __EXAMPLE_INTERRUPT_H__
#define __EXAMPLE_INTERRUPT_H__

void example_interrupt_init(void);
void example_interrupt_loop(void);
void example_interrupt_on_drdy(void);

#endif /* __EXAMPLE_INTERRUPT_H__ */
