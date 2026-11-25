#ifndef PORT_CONFIG_H
#define PORT_CONFIG_H

/*
 * Interrupt priority configuration shared by the C++ port and the assembly files.
 * Use plain integer literals only (no 'u' suffixes) so the GNU assembler accepts them.
 */

/* Number of implemented NVIC priority bits (STM32H7: __NVIC_PRIO_BITS = 4). */
#define PORT_NVIC_PRIO_BITS                 4

/*
 * Highest (numerically lowest) logical interrupt priority that may call kernel API functions.
 * ISRs with a logical priority 0 .. (PORT_MAX_SYSCALL_INTERRUPT_PRIORITY - 1) are never masked
 * by kernel critical sections and must not call into the kernel.
 */
#define PORT_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/*
 * Value written to BASEPRI to enter a kernel critical section.
 * Only the upper PORT_NVIC_PRIO_BITS bits of BASEPRI are implemented, so the logical
 * priority must be shifted into place (5 -> 0x50). Writing the unshifted value would
 * result in BASEPRI = 0, which disables masking completely.
 */
#define PORT_BASEPRI_SYSCALL                (PORT_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - PORT_NVIC_PRIO_BITS))

#endif /* PORT_CONFIG_H */
