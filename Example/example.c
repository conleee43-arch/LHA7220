/**
 * @file example.c
 * @brief LHA7220 测试例程统一分发实现
 */

#include "example.h"
#include "example-polling.h"
#include "example-interrupt.h"
#include "example-temperature.h"
#include "example-multichannel.h"
#include "example-single-shot.h"
#include "example-reg-test.h"
#include "LHA7220-CMD.h"

void example_init(void)
{
#if (ACTIVE_EXAMPLE == EXAMPLE_POLLING)
    example_polling_init();
#elif (ACTIVE_EXAMPLE == EXAMPLE_INTERRUPT)
    example_interrupt_init();
#elif (ACTIVE_EXAMPLE == EXAMPLE_TEMPERATURE)
    example_temperature_init();
#elif (ACTIVE_EXAMPLE == EXAMPLE_MULTICHANNEL)
    example_multichannel_init();
#elif (ACTIVE_EXAMPLE == EXAMPLE_SINGLE_SHOT)
    example_single_shot_init();
#elif (ACTIVE_EXAMPLE == EXAMPLE_REG_TEST)
    example_reg_test_init();
#elif (ACTIVE_EXAMPLE == EXAMPLE_CMD)
    lha7220_cmd_init();
#else
    #error "Please select a valid ACTIVE_EXAMPLE in example.h!"
#endif
}

void example_loop(void)
{
#if (ACTIVE_EXAMPLE == EXAMPLE_POLLING)
    example_polling_loop();
#elif (ACTIVE_EXAMPLE == EXAMPLE_INTERRUPT)
    example_interrupt_loop();
#elif (ACTIVE_EXAMPLE == EXAMPLE_TEMPERATURE)
    example_temperature_loop();
#elif (ACTIVE_EXAMPLE == EXAMPLE_MULTICHANNEL)
    example_multichannel_loop();
#elif (ACTIVE_EXAMPLE == EXAMPLE_SINGLE_SHOT)
    example_single_shot_loop();
#elif (ACTIVE_EXAMPLE == EXAMPLE_REG_TEST)
    example_reg_test_loop();
#elif (ACTIVE_EXAMPLE == EXAMPLE_CMD)
    lha7220_cmd_loop();
#endif
}

void example_on_drdy_irq(void)
{
#if (ACTIVE_EXAMPLE == EXAMPLE_INTERRUPT)
    example_interrupt_on_drdy();
#elif (ACTIVE_EXAMPLE == EXAMPLE_CMD)
    lha7220_cmd_on_drdy_irq();
#endif
}
