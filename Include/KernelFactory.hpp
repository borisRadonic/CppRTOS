#pragma once

#include "Kernel.hpp"

namespace CppRtos
{

	class KernelFactory
	{
	
	public:

		// Prevent creating multiple instances of the KernelFactory
		//KernelFactory(const KernelFactory&) = delete;
		//KernelFactory& operator=(const KernelFactory&) = delete;

		inline static KernelFactory& getInstance() noexcept
		{
			static KernelFactory instanceFactory; // This creates a single instance on first use
			return instanceFactory;
		}

		inline Kernel* getKernel() noexcept
		{
			return kernel;
		}

		Kernel* create( void* platformMemory ) noexcept
		{
			if (_isCreated)
			{
				return nullptr;
			}
			_isCreated = true;
			kernel = new(platformMemory) Kernel();
			return kernel;
		}

		void destroy( void* platformMemory )
		{
			if (_isCreated)
			{
				static_cast<Kernel*>(platformMemory)->~Kernel();
				_isCreated = false;
			}
		}
	private:

		bool _isCreated = false;
		Kernel* kernel = nullptr;

		KernelFactory() = default;
		~KernelFactory() = default;
		KernelFactory(const KernelFactory&) = delete;
		KernelFactory& operator=(const KernelFactory&) = delete;
		KernelFactory(KernelFactory&&) = delete;
		KernelFactory& operator=(KernelFactory&&) = delete;

	};
}