.cpu cortex-m7
.arch armv7e-m
.fpu fpv5-sp-d16
.thumb
.syntax unified

#define MAX_SYSCALL_INTERRUPT_PRIORITY 5u

.global startInitialTask
.thumb_func
.type startInitialTask, %function
startInitialTask:
    ldr r0, =0xE000ED08   // Load the address of the VTOR register
    ldr r0, [r0]          // Dereference to get the address of the vector table
    ldr r0, [r0]          // Dereference again to get the initial stack pointer value

    msr msp, r0           // Set the MSP back to the start of the stack
    mov r0, #0            // Clear the CONTROL register (set to privileged mode, use MSP)
    msr control, r0
    cpsie i               // Enable interrupts
    cpsie f               // Enable fault interrupts
    dsb                   // Data Synchronization Barrier
    isb                   // Instruction Synchronization Barrier
    svc 0                 // System call to start first task
    nop                   // No operation (placeholder)
    .ltorg                // Literal pool location directive

    .align 4              // Align the next data on a 4-byte boundary
    .size startInitialTask, .-startInitialTask


.global setPrivilegedMode
.thumb_func
.type setPrivilegedMode, %function
setPrivilegedMode:
    mrs r0, control       // Read the current value of the CONTROL register
    bic r0, r0, #1        // Clear the least significant bit to select privileged mode
    msr control, r0       // Write the modified value back to the CONTROL register
    isb                   // Ensure the change takes effect immediately
    bx lr                 // Return from function
    .size setPrivilegedMode, .-setPrivilegedMode


.global setInterruptPriorityAndGetOriginal
.thumb_func
.type setInterruptPriorityAndGetOriginal, %function
setInterruptPriorityAndGetOriginal:
    mrs r0, basepri                      // Read the current value of the BASEPRI register into r0
    mov r1, #MAX_SYSCALL_INTERRUPT_PRIORITY  // Move the value of MAX_SYSCALL_INTERRUPT_PRIORITY into r1
    cpsid i                              // Disable interrupts
    msr basepri, r1                      // Set the BASEPRI register to the new value
    isb                                  // Instruction Synchronization Barrier
    dsb                                  // Data Synchronization Barrier
    cpsie i                              // Enable interrupts
    bx lr                                // Return from function
    .size setInterruptPriorityAndGetOriginal, .-setInterruptPriorityAndGetOriginal


.global setInterruptBasePriority
.thumb_func
.type setInterruptBasePriority, %function
setInterruptBasePriority:
    msr basepri, r0       // Set the BASEPRI register to the value in r0
    bx lr                 // Return from function
    .size setInterruptBasePriority, .-setInterruptBasePriority


.global enableVFP
.thumb_func
.type enableVFP, %function
enableVFP:
    ldr.w r0, =0xE000ED88       // The FPU enable bits are in the CPACR.
    ldr r1, [r0]                // Load the CPACR value
    orr r1, r1, #( 0xf << 20 )  // Enable CP10 and CP11 coprocessors
    str r1, [r0]                // Store the updated CPACR value
    bx r14                      // Return from function
    .ltorg                      // Literal pool location directive
    .size enableVFP, .-enableVFP

