#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

namespace CppRtos
{
	enum class TaskStateType : std::uint8_t
	{
		eReady			= 0u,
		eRunning		= 1u,
		eBlocked		= 2u,
		eSuspended		= 3u
	};

    class TaskState
    {
    public:

        TaskState(): _state(TaskStateType::eReady)
        {
        }

        TaskState(TaskStateType initialState = TaskStateType::eReady)
            : _state(initialState)
        {
        }

        inline void setState(TaskStateType newState)
        {
            _state = newState;
        }

        inline TaskStateType getState() const
        {
            return _state;
        }
       

    private:
        TaskStateType _state;
    };
    /*
    template<std::size_t REGISTER_COUNT, std::size_t STACK_SIZE>
    class TaskContext
    {
    public:

        TaskContext() : stackPointer(nullptr), programCounter(nullptr)
        {
            registers.fill(0);
        }

        void saveContext(uint32_t* sp, void (*pc)(), const std::array<uint32_t, REGISTER_COUNT>& regs)
        {
            stackPointer = sp;
            programCounter = pc;
            registers = regs;
        }

        void restoreContext(uint32_t*& sp, void (*&pc)(), std::array<uint32_t, REGISTER_COUNT>& regs) const
        {
            sp = stackPointer;
            pc = programCounter;
            regs = registers;
        }

        void setStackPointer(uint32_t* sp)
        {
            stackPointer = sp;
        }

        uint32_t* getStackPointer() const
        {
            return stackPointer;
        }

        void setProgramCounter(void (*pc)())
        {
            programCounter = pc;
        }

        void (*getProgramCounter() const)()
        {
            return programCounter;
        }

        void setRegisters(const std::array<uint32_t, REGISTER_COUNT>& regs)
        {
            registers = regs;
        }

        std::array<uint32_t, REGISTER_COUNT> getRegisters() const
        {
            return registers;
        }

    private:
        std::array<uint32_t, STACK_SIZE> stack = {};
        uint32_t* stackPointer = nullptr;
        void (*programCounter)();
        std::array<uint32_t, REGISTER_COUNT> registers;
        
    };
    */

    template<std::size_t REGISTER_COUNT, std::size_t STACK_SIZE>
    class Task
    {
    public:
        Task(int priority)
            : _priority(priority), _state(TaskStateType::eReady)
        {
        }

        virtual ~Task() = default;

        virtual void run() = 0;

        inline void suspend()
        {
            _state.setState(TaskStateType::eSuspended);
        }

        inline void resume()
        {
            _state.setState(TaskStateType::eReady);
        }

        inline void block()
        {
            _state.setState(TaskStateType::eBlocked);
        }

        inline void unblock()
        {
            _state.setState(TaskStateType::eReady);
        }

        inline void setPriority(std::uint32_t priority)
        {
            _priority = priority;
        }

        inline int getPriority() const
        {
            return _priority;
        }

        inline TaskStateType getState() const
        {
            return _state.getState();
        }

        /*
        inline TaskContext<REGISTER_COUNT, STACK_SIZE>& getContext()
        {
            return _context;
        }
        */

    private:
        std::uint32_t               _priority;
        TaskState                   _state;
        //TaskContext<REGISTER_COUNT, STACK_SIZE> _context;

        std::array<uint32_t, STACK_SIZE> stack = {};
        uint32_t* stackPointer = nullptr;
        void (*programCounter)();
        std::array<uint32_t, REGISTER_COUNT> registers;



    };
}