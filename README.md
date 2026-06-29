# CppRTOS

**Modern Real-Time Operating System written in C++ for ARM Cortex-M microcontrollers**

CppRTOS is a lightweight, preemptive real-time operating system written entirely in modern C++ for ARM Cortex-M microcontrollers.

The project demonstrates that object-oriented C++ can be used to build deterministic, high-performance embedded systems without sacrificing predictability or efficiency.

Unlike traditional C-based RTOS implementations, CppRTOS provides a clean, type-safe API while maintaining low runtime overhead and deterministic execution.

**Project Status:** Preview Release (Core functionality implemented, API is still evolving.)

---

# Features

- Modern C++ API
- Preemptive priority-based scheduler
- Deterministic context switching
- Static object allocation
- Task abstraction using C++ classes
- Message Queues
- Semaphores
- Alarms
- Mutexes
- Counters
- Event Flags
- Idle Task
- Portable hardware abstraction layer
- Zero dynamic memory required
- Optimized for ARM Cortex-M


---

# Design Goals

CppRTOS has been designed with the following objectives:

- Deterministic execution
- Small memory footprint
- Strong type safety
- Clean object-oriented API
- Zero-cost abstractions where possible
- Easy portability
- Minimal dependencies
- High readability and maintainability

---

# Architecture

```text
+------------------------------------------------------+
|                 User Application                     |
+------------------------------------------------------+

        Task      Queue      Mutex      Semaphore

+------------------------------------------------------+
|                  CppRTOS API                         |
+------------------------------------------------------+

| Scheduler | Kernel | IPC | Synchronization | Timing |

+------------------------------------------------------+
|      Cortex-M Port / Context Switching Layer         |
+------------------------------------------------------+

| SysTick | PendSV | SVC | Interrupt Management |

+------------------------------------------------------+
|                 ARM Cortex-M MCU                     |
+------------------------------------------------------+



# Why C++?

Modern C++ offers several advantages for embedded real-time systems:

* Strong type safety
* Better encapsulation
* Improved code reuse
* Compile-time optimizations
* Cleaner interfaces
* Easier maintenance
* Reduced programming errors

CppRTOS demonstrates that modern C++ can be used for hard real-time embedded applications while preserving deterministic execution.

---

# Current Status

| Component          | Status         |
| ------------------ | -------------- |
| Scheduler          | ✅              |
| Context Switching  | ✅              |
| Tasks              | ✅              |
| Message Queue      | ✅              |
| Binary Semaphore   | ✅              |
| Counting Semaphore | ✅              |
| Mutex              | ✅              |
| Event Flags        | ✅              |
| Idle Task          | ✅              |
| Software Timers    | ✅             |
| SMP Support        | Not Planned    |

---

# Supported Platforms

Current architecture:

* ARM Cortex-M7


Additional Cortex-M devices can be supported by implementing a platform-specific port.

---

# Design Philosophy

CppRTOS intentionally avoids unnecessary complexity.

The focus is on:

* deterministic execution
* simplicity
* portability
* maintainability
* modern C++ design

rather than implementing every possible RTOS feature.

---

# Roadmap

* [x] Kernel
* [x] Scheduler
* [x] Context Switching
* [x] Task Management
* [x] Message Queues
* [x] Synchronization Primitives
* [x] Software Timers
* [x] Unit Tests
* [ ] Additional Examples
* [ ] API Documentation

---


## Example

Creating a task is straightforward by deriving from `CppRtos::Task`.

```cpp
class WorkerTask : public CppRtos::Task
{
public:

    WorkerTask(void* stack, size_t stackSize)
        : Task(stack, stackSize)
    {
        setName("Worker");
        setPriority(TaskPriority::PRIORITY_NORMAL);
    }

private:

    void run() override
    {
        while (true)
        {
            // Application code

            sleep(100);
        }
    }
};

alignas(8) static uint8_t workerStack[2048];

WorkerTask worker(workerStack, sizeof(workerStack));

int main()
{
    auto& kernelFactory = CppRtos::KernelFactory::getInstance();

    CppRtos::Kernel* kernel =
        kernelFactory.create(kernelMemory);

    kernel->addTask(worker);

    kernel->start();
}
```

# Building

Support for CMake will be added in a future release.

Currently the project is developed using CLion

---

# License

MIT License

---

# Author

**Boris Radonic**

Embedded Software Engineer

Areas of expertise:

* Embedded Systems
* Real-Time Software
* HW Design/ Power Electronic
* Motor Control
* Functional Safety
* Modern C++

```
```
