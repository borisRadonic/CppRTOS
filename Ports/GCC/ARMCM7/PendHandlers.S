.cpu cortex-m7
.arch armv7e-m
.fpu fpv5-sp-d16
.thumb
.syntax unified

#define MAX_SYSCALL_INTERRUPT_PRIORITY 5u

.global PendSV_Handler
.thumb_func
.type PendSV_Handler, %function
PendSV_Handler:
    mrs r0, psp                        // Move the current value of the Process Stack Pointer (PSP) into r0
    isb                                // Instruction Synchronization Barrier to ensure subsequent instructions use updated values

    ldr r3, ptrCurrentTaskConst        // Load the address of ptrCurrentTask into r3
    ldr r2, [r3]                       // Load the value of pxCurrentTCB (i.e., the current TCB) into r2

    tst r14, #0x10                     // Test if bit 4 of r14 (EXC_RETURN) is set (indicating FPU context)
    it eq                              // If the zero flag is set (result of tst is zero), execute the next instruction
    vstmdbeq r0!, {s16-s31}            // If using FPU context, store the high VFP registers (s16-s31) and decrement r0

    stmdb r0!, {r4-r11, r14}           // Store multiple registers (r4-r11, r14) and decrement r0
    str r0, [r2]                       // Store the updated stack pointer (r0) into the current TCB (pointed to by r2)

    stmdb sp!, {r0, r3}                // Store r0 and r3 on the stack and decrement SP
    mov r0, #MAX_SYSCALL_INTERRUPT_PRIORITY // Move the value of configMAX_SYSCALL_INTERRUPT_PRIORITY into r0
    msr basepri, r0                    // Move the value of r0 into the BASEPRI register (set interrupt priority)
    dsb                                // Data Synchronization Barrier to ensure all memory accesses complete
    isb                                // Instruction Synchronization Barrier to ensure subsequent instructions use updated values
    bl taskSwitchContext               // Branch to the taskSwitchContext function (perform a context switch)
    mov r0, #0                         // Clear r0
    msr basepri, r0                    // Clear the BASEPRI register (reset interrupt priority)
    ldmia sp!, {r0, r3}                // Load multiple registers (r0 and r3) from the stack and increment SP

    ldr r1, [r3]                       // Load the value at the address in r3 (i.e., the new TCB) into r1
    ldr r0, [r1]                       // Load the stack pointer of the new task from the new TCB (pointed to by r1) into r0

    ldmia r0!, {r4-r11, r14}           // Load multiple registers (r4-r11, r14) from the stack and increment r0

    tst r14, #0x10                     // Test if bit 4 of r14 (EXC_RETURN) is set (indicating FPU context)
    it eq                              // If the zero flag is set (result of tst is zero), execute the next instruction
    vldmiaeq r0!, {s16-s31}            // If using FPU context, load the high VFP registers (s16-s31) and increment r0

    msr psp, r0                        // Move the updated stack pointer (r0) into the Process Stack Pointer (PSP)
    isb                                // Instruction Synchronization Barrier to ensure subsequent instructions use updated values

    bx r14                             // Branch to the address in r14 (return from exception)

    .align 4                           // Align the next data on a 4-byte boundary
ptrCurrentTaskConst: .word ptrCurrentTask // Define a word containing the address of pxCurrentTCB

.global SVC_Handler
.thumb_func
.type SVC_Handler, %function
SVC_Handler:
    ldr r3, =currentTaskDataAddr    // Load the address of currentTaskDataAddr
    ldr r1, [r3]                    // Use currentTaskDataAddr to get the current task data address
    ldr r0, [r1]                    // The first item in the current task data is the task top of stack

    ldmia r0!, {r4-r11, r14}        // Pop the registers that are not automatically saved on exception entry and the critical nesting count
    msr psp, r0                     // Restore the task stack pointer
    isb                             // Instruction Synchronization Barrier
    mov r0, #0                      // Clear the BASEPRI register
    msr basepri, r0                 // Restore interrupts

    bx r14                          // Return from the exception handler

    .align 4                        // Align the next data on a 4-byte boundary
    .size SVC_Handler, .-SVC_Handler