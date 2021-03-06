//****************************************************************************
// Product: DPP example, STM3210C-EVAL board, QK kernel
// Last Updated for Version: 5.1.0
// Date of the Last Update:  Oct 01, 2013
//
//                    Q u a n t u m     L e a P s
//                    ---------------------------
//                    innovating embedded systems
//
// Copyright (C) 2002-2013 Quantum Leaps, LLC. All rights reserved.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Alternatively, this program may be distributed and modified under the
// terms of Quantum Leaps commercial licenses, which expressly supersede
// the GNU General Public License and are specifically designed for
// licensees interested in retaining the proprietary status of their code.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// Contact information:
// Quantum Leaps Web sites: http://www.quantum-leaps.com
//                          http://www.state-machine.com
// e-mail:                  info@quantum-leaps.com
//****************************************************************************
#include "qp_port.h"
#include "dpp.h"
#include "bsp.h"

extern "C" {
    #include "stm32f10x.h"
    #include "stm32_eval.h"
    #include "stm3210c_eval_lcd.h"
}

//****************************************************************************
namespace DPP {

Q_DEFINE_THIS_FILE

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CAUTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Assign a priority to EVERY ISR explicitly by calling NVIC_SetPriority().
// DO NOT LEAVE THE ISR PRIORITIES AT THE DEFAULT VALUE!
//
enum KernelUnawareISRs {                                         // see NOTE00
    // ...
    MAX_KERNEL_UNAWARE_CMSIS_PRI                           // keep always last
};
// "kernel-unaware" interrupts can't overlap "kernel-aware" interrupts
Q_ASSERT_COMPILE(MAX_KERNEL_UNAWARE_CMSIS_PRI <= QF_AWARE_ISR_CMSIS_PRI);

enum KernelAwareISRs {
    EXTI0_PRIO = QF_AWARE_ISR_CMSIS_PRI,                         // see NOTE00
    SYSTICK_PRIO,
    // ...
    MAX_KERNEL_AWARE_CMSIS_PRI                             // keep always last
};
// "kernel-aware" interrupts should not overlap the PendSV priority
Q_ASSERT_COMPILE(MAX_KERNEL_AWARE_CMSIS_PRI <= (0xFF >>(8-__NVIC_PRIO_BITS)));

//............................................................................
static unsigned  l_rnd;                                         // random seed

#ifdef Q_SPY
    QP::QSTimeCtr QS_tickTime_;
    QP::QSTimeCtr QS_tickPeriod_;
    static uint8_t l_SysTick_Handler;

    #define QS_BUF_SIZE   (2*1024)
    #define QS_BAUD_RATE  115200

    enum AppRecords {                    // application-specific trace records
        PHILO_STAT = QP::QS_USER
    };
#endif

//............................................................................
extern "C" void SysTick_Handler(void) __attribute__((__interrupt__));
extern "C" void SysTick_Handler(void) {
    QK_ISR_ENTRY();                          // inform QK-nano about ISR entry
#ifdef Q_SPY
    uint32_t dummy = SysTick->CTRL;           // clear NVIC_ST_CTRL_COUNT flag
    QS_tickTime_ += QS_tickPeriod_;          // account for the clock rollover
#endif

    QP::QF::TICK(&l_SysTick_Handler);         // process all armed time events

    QK_ISR_EXIT();                            // inform QK-nano about ISR exit
}
//............................................................................
extern "C" void EXTI0_IRQHandler(void) __attribute__((__interrupt__));
extern "C" void EXTI0_IRQHandler(void) {
    QK_ISR_ENTRY();                          // inform QK-nano about ISR entry
    EXTI->PR = 0x1;      // set the EXTI->PR[0] to clear the EXTI_SWIER[0] bit

    AO_Table->POST(Q_NEW(QP::QEvt, MAX_PUB_SIG), (void *)0);    // for testing

    QK_ISR_EXIT();                            // inform QK-nano about ISR exit
}

//............................................................................
void BSP_init(void) {

    SystemInit();            // initialize STM32 system (clock, PLL and Flash)

                // initialize LEDs, Key Button, and LCD on STM3210X-EVAL board
    STM_EVAL_LEDInit(LED1);
    STM_EVAL_LEDInit(LED2);
    STM_EVAL_LEDInit(LED3);
    STM_EVAL_LEDInit(LED4);

                       // initialize the EXTI Line0 interrupt used for testing
    EXTI_InitTypeDef exti_init;
    exti_init.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = EXTI_Trigger_Rising;
    exti_init.EXTI_Line    = EXTI_Line0;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);

    STM3210C_LCD_Init();                                 // initialize the LCD
    LCD_Clear(White);                                         // clear the LCD
    LCD_SetBackColor(Grey);
    LCD_SetTextColor(Black);
    LCD_DisplayString(Line0, 0, "   Quantum Leaps    ");
    LCD_DisplayString(Line1, 0, "     DPP example    ");
    LCD_DisplayString(Line2, 0, " QP/C++ (QK)        ");
    LCD_DisplayString(Line2, 14*16, QP::QF::getVersion());
    LCD_SetBackColor(White);
    LCD_DisplayString(Line5, 0, "DPP:");
    LCD_SetBackColor(Black);
    LCD_SetTextColor(Yellow);
    LCD_DisplayString(Line9, 0, "  state-machine.com ");
    LCD_SetBackColor(Blue);
    LCD_SetTextColor(White);
    LCD_DisplayString(Line5, 4*16, "0 ,1 ,2 ,3 ,4    ");

    if (QS_INIT((void *)0) == 0) {       // initialize the QS software tracing
        Q_ERROR();
    }
    QS_OBJ_DICTIONARY(&l_SysTick_Handler);
}
//............................................................................
// error routine that is called if the STM32 library encounters an error
extern "C" void assert_failed(char const *file, int line) {
    Q_onAssert(file, line);
}
//............................................................................
extern "C" void Q_onAssert(char const * const file, int line) {
    (void)file;                                      // avoid compiler warning
    (void)line;                                      // avoid compiler warning
    QF_INT_DISABLE();            // make sure that all interrupts are disabled
    for (;;) {          // NOTE: replace the loop with reset for final version
    }
}
//............................................................................
void BSP_terminate(int16_t result) {
    (void)result;
}
//............................................................................
void BSP_displayPhilStat(uint8_t n, char const *stat) {

    LCD_DisplayChar(Line5, (3*16*n + 5*16), stat[0]);

    QS_BEGIN(PHILO_STAT, AO_Philo[n])     // application-specific record begin
        QS_U8(1, n);                                     // Philosopher number
        QS_STR(stat);                                    // Philosopher status
    QS_END()
}
//............................................................................
void BSP_displayPaused(uint8_t paused) {
    (void)paused;
}
//............................................................................
uint32_t BSP_random(void) {     // a very cheap pseudo-random-number generator
    // "Super-Duper" Linear Congruential Generator (LCG)
    // LCG(2^32, 3*7*11*13*23, 0, seed)
    //
    l_rnd = l_rnd * (3*7*11*13*23);
    return l_rnd >> 8;
}
//............................................................................
void BSP_randomSeed(uint32_t seed) {
    l_rnd = seed;
}

}                                                             // namespace DPP
//****************************************************************************


namespace QP {

//............................................................................
void QF::onStartup(void) {
    // Set up and enable the SysTick timer.  It will be used as a reference
    // for delay loops in the interrupt handlers.  The SysTick timer period
    // will be set up for BSP_TICKS_PER_SEC.
    //
    SysTick_Config(SystemFrequency_SysClk / DPP::BSP_TICKS_PER_SEC);

    // assing all priority bits for preemption-prio. and none to sub-prio.
    NVIC_SetPriorityGrouping(0U);

    // set priorities of ALL ISRs used in the system, see NOTE00
    //
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!! CAUTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Assign a priority to EVERY ISR explicitly by calling NVIC_SetPriority().
    // DO NOT LEAVE THE ISR PRIORITIES AT THE DEFAULT VALUE!
    //
    NVIC_SetPriority(SysTick_IRQn, DPP::SYSTICK_PRIO);
    NVIC_SetPriority(EXTI0_IRQn,   DPP::EXTI0_PRIO);
    // ...
                                                             // enable IRQs...
    NVIC_EnableIRQ(EXTI0_IRQn);
}
//............................................................................
void QF::onCleanup(void) {
}
//............................................................................
void QK::onIdle(void) {
                            // toggle the blue LED on and then off, see NOTE01
    QF_INT_DISABLE();
    STM_EVAL_LEDOn (LED4);                                     // blue LED on
    STM_EVAL_LEDOff(LED4);                                     // blue LED off
    QF_INT_ENABLE();

#ifdef Q_SPY

    if ((USART2->SR & USART_FLAG_TXE) != 0) {                 // is TXE empty?

        QF_INT_DISABLE();
        uint16_t b = QS::getByte();
        QF_INT_ENABLE();

        if (b != QS_EOD) {                                 // not End-Of-Data?
           USART2->DR = (b & 0xFF);                // put into the DR register
        }
    }

#elif defined NDEBUG
    __WFI();                                             // wait for interrupt
#endif
}

//----------------------------------------------------------------------------
#ifdef Q_SPY
//............................................................................
bool QS::onStartup(void const *arg) {
    static uint8_t qsBuf[QS_BUF_SIZE];               // buffer for Quantum Spy
    initBuf(qsBuf, sizeof(qsBuf));

                                        // enable USART2 and GPIOA/AFIO clocks
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO,
                           ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);

                                          // configure GPIOD.5 as push-pull...
    GPIO_InitTypeDef  gpio_init;
    gpio_init.GPIO_Pin   = GPIO_Pin_5;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &gpio_init);
                                     // configure GPIOD.6 as input floating...
    gpio_init.GPIO_Pin   = GPIO_Pin_6;
    gpio_init.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &gpio_init);

    USART_InitTypeDef usart_init;
    usart_init.USART_BaudRate            = QS_BAUD_RATE;
    usart_init.USART_WordLength          = USART_WordLength_8b;
    usart_init.USART_StopBits            = USART_StopBits_1;
    usart_init.USART_Parity              = USART_Parity_No ;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode                = USART_Mode_Tx;
    USART_Init(USART2, &usart_init);

    USART_ClockInitTypeDef usart_clk_init;
    usart_clk_init.USART_Clock               = USART_Clock_Disable;
    usart_clk_init.USART_CPOL                = USART_CPOL_Low;
    usart_clk_init.USART_CPHA                = USART_CPHA_2Edge;
    usart_clk_init.USART_LastBit             = USART_LastBit_Disable;
    USART_ClockInit(USART2, &usart_clk_init);

    USART_Cmd(USART2, ENABLE);                                // enable USART2

    DPP::QS_tickPeriod_ =
        (QSTimeCtr)(SystemFrequency_SysClk / DPP::BSP_TICKS_PER_SEC);
    DPP::QS_tickTime_ = DPP::QS_tickPeriod_;           // to start the timestamp at zero

                                                 /* setup the QS filters... */
    QS_FILTER_ON(QS_ALL_RECORDS);

//    QS_FILTER_OFF(QS_QEP_STATE_EMPTY);
//    QS_FILTER_OFF(QS_QEP_STATE_ENTRY);
//    QS_FILTER_OFF(QS_QEP_STATE_EXIT);
//    QS_FILTER_OFF(QS_QEP_STATE_INIT);
//    QS_FILTER_OFF(QS_QEP_INIT_TRAN);
//    QS_FILTER_OFF(QS_QEP_INTERN_TRAN);
//    QS_FILTER_OFF(QS_QEP_TRAN);
//    QS_FILTER_OFF(QS_QEP_IGNORED);

    QS_FILTER_OFF(QS_QF_ACTIVE_ADD);
    QS_FILTER_OFF(QS_QF_ACTIVE_REMOVE);
    QS_FILTER_OFF(QS_QF_ACTIVE_SUBSCRIBE);
    QS_FILTER_OFF(QS_QF_ACTIVE_UNSUBSCRIBE);
    QS_FILTER_OFF(QS_QF_ACTIVE_POST_FIFO);
    QS_FILTER_OFF(QS_QF_ACTIVE_POST_LIFO);
    QS_FILTER_OFF(QS_QF_ACTIVE_GET);
    QS_FILTER_OFF(QS_QF_ACTIVE_GET_LAST);
    QS_FILTER_OFF(QS_QF_EQUEUE_INIT);
    QS_FILTER_OFF(QS_QF_EQUEUE_POST_FIFO);
    QS_FILTER_OFF(QS_QF_EQUEUE_POST_LIFO);
    QS_FILTER_OFF(QS_QF_EQUEUE_GET);
    QS_FILTER_OFF(QS_QF_EQUEUE_GET_LAST);
    QS_FILTER_OFF(QS_QF_MPOOL_INIT);
    QS_FILTER_OFF(QS_QF_MPOOL_GET);
    QS_FILTER_OFF(QS_QF_MPOOL_PUT);
    QS_FILTER_OFF(QS_QF_PUBLISH);
    QS_FILTER_OFF(QS_QF_NEW);
    QS_FILTER_OFF(QS_QF_GC_ATTEMPT);
    QS_FILTER_OFF(QS_QF_GC);
//    QS_FILTER_OFF(QS_QF_TICK);
    QS_FILTER_OFF(QS_QF_TIMEEVT_ARM);
    QS_FILTER_OFF(QS_QF_TIMEEVT_AUTO_DISARM);
    QS_FILTER_OFF(QS_QF_TIMEEVT_DISARM_ATTEMPT);
    QS_FILTER_OFF(QS_QF_TIMEEVT_DISARM);
    QS_FILTER_OFF(QS_QF_TIMEEVT_REARM);
    QS_FILTER_OFF(QS_QF_TIMEEVT_POST);
    QS_FILTER_OFF(QS_QF_CRIT_ENTRY);
    QS_FILTER_OFF(QS_QF_CRIT_EXIT);
    QS_FILTER_OFF(QS_QF_ISR_ENTRY);
    QS_FILTER_OFF(QS_QF_ISR_EXIT);

//    QS_FILTER_OFF(QS_QK_MUTEX_LOCK);
//    QS_FILTER_OFF(QS_QK_MUTEX_UNLOCK);
    QS_FILTER_OFF(QS_QK_SCHEDULE);

    return true;                                             // return success
}
//............................................................................
void QS::onCleanup(void) {
}
//............................................................................
QSTimeCtr QS::onGetTime(void) {              // invoked with interrupts locked
    if ((SysTick->CTRL & 0x00010000U) == 0U) {                // COUNT no set?
        return DPP::QS_tickTime_ - (QSTimeCtr)SysTick->VAL;
    }
    else {        // the rollover occured, but the SysTick_ISR did not run yet
        return DPP::QS_tickTime_ - (QSTimeCtr)SysTick->VAL
               + DPP::QS_tickPeriod_;
    }
}
//............................................................................
void QS::onFlush(void) {
    uint16_t b;
    while ((b = getByte()) != QS_EOD) {            // while not End-Of-Data...
        while ((USART2->SR & USART_FLAG_TXE) == 0) {    // while TXE not empty
        }
        USART2->DR = (b & 0xFF);                   // put into the DR register
    }
}
#endif                                                                // Q_SPY
//----------------------------------------------------------------------------

}                                                              // namespace QP

//****************************************************************************
// NOTE00:
// The QF_AWARE_ISR_CMSIS_PRI constant from the QF port specifies the highest
// ISR priority that is disabled by the QF framework. The value is suitable
// for the NVIC_SetPriority() CMSIS function.
//
// Only ISRs prioritized at or below the QF_AWARE_ISR_CMSIS_PRI level (i.e.,
// with the numerical values of priorities equal or higher than
// QF_AWARE_ISR_CMSIS_PRI) are allowed to call the QK_ISR_ENTRY/QK_ISR_ENTRY
// macros or any other QF/QK  services. These ISRs are "QF-aware".
//
// Conversely, any ISRs prioritized above the QF_AWARE_ISR_CMSIS_PRI priority
// level (i.e., with the numerical values of priorities less than
// QF_AWARE_ISR_CMSIS_PRI) are never disabled and are not aware of the kernel.
// Such "QF-unaware" ISRs cannot call any QF/QK services. In particular they
// can NOT call the macros QK_ISR_ENTRY/QK_ISR_ENTRY. The only mechanism
// by which a "QF-unaware" ISR can communicate with the QF framework is by
// triggering a "QF-aware" ISR, which can post/publish events.
//
// NOTE01:
// The blue LED is used to visualize the idle loop activity. The brightness
// of the LED is proportional to the frequency of invcations of the idle loop.
// Please note that the LED is toggled with interrupts locked, so no interrupt
// execution time contributes to the brightness of the User LED.
//
