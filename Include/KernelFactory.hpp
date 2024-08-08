#pragma once

#include "Kernel.hpp"

namespace CppRtos
{
	/**
    * @brief Singleton factory class for creating and managing a Kernel instance.
    *
    * The KernelFactory class is responsible for ensuring that only one instance of the Kernel
    * is created and managed during the lifetime of the application. It provides methods to
    * create, retrieve, and destroy the Kernel instance, using a singleton pattern.
    */
    class KernelFactory
    {
    public:
        /**
         * @brief Deleted copy constructor to prevent copying of the KernelFactory instance.
         */
        KernelFactory(const KernelFactory&) = delete;

        /**
         * @brief Deleted copy assignment operator to prevent copying of the KernelFactory instance.
         */
        KernelFactory& operator=(const KernelFactory&) = delete;

        /**
         * @brief Deleted move constructor to prevent moving of the KernelFactory instance.
         */
        KernelFactory(KernelFactory&&) = delete;

        /**
         * @brief Deleted move assignment operator to prevent moving of the KernelFactory instance.
         */
        KernelFactory& operator=(KernelFactory&&) = delete;

        /**
         * @brief Retrieves the singleton instance of KernelFactory.
         *
         * This method ensures that only one instance of KernelFactory exists. The instance is created
         * on the first call and subsequently returned on all future calls.
         *
         * @return Reference to the singleton instance of KernelFactory.
         */
        inline static KernelFactory& getInstance() noexcept
        {
            static KernelFactory instanceFactory; // This creates a single instance on first use
            return instanceFactory;
        }

        /**
         * @brief Retrieves the current Kernel instance.
         *
         * This method returns the Kernel instance managed by the KernelFactory. If the Kernel has
         * not been created yet, it will return nullptr.
         *
         * @return Pointer to the Kernel instance, or nullptr if not created.
         */
        inline Kernel* getKernel() noexcept
        {
            return ptrKernel;
        }

        /**
         * @brief Creates the Kernel instance using the provided memory.
         *
         * This method creates the Kernel instance in the specified memory region. It can only be called
         * once; subsequent calls will return nullptr if the Kernel is already created.
         *
         * @param platformMemory Pointer to the memory where the Kernel should be created.
         * @return Pointer to the newly created Kernel instance, or nullptr if the Kernel was already created.
         */
        Kernel* create(void* platformMemory) noexcept
        {
            if (isCreated)
            {
                return nullptr;
            }
            isCreated = true;
            ptrKernel = new(platformMemory) Kernel();
            return ptrKernel;
        }

        /**
         * @brief Destroys the Kernel instance.
         *
         * This method destroys the Kernel instance by explicitly calling its destructor. It also resets
         * the creation flag, allowing the Kernel to be created again if needed.
         *
         * @param platformMemory Pointer to the memory where the Kernel is stored.
         */
        void destroy(void* platformMemory)
        {
            if (isCreated)
            {
                static_cast<Kernel*>(platformMemory)->~Kernel();
                isCreated = false;
            }
        }

    private:
        /**
         * @brief Default constructor for KernelFactory.
         *
         * The constructor is private to enforce the singleton pattern.
         */
        KernelFactory() = default;

        /**
         * @brief Default destructor for KernelFactory.
         */
        ~KernelFactory() = default;

        bool isCreated = false; /**< Flag indicating whether the Kernel has been created. */

        Kernel* ptrKernel = nullptr; /**< Pointer to the Kernel instance managed by the factory. */
    };
}