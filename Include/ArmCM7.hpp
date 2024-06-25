#pragma once

#include <cstdint>
#include <cstddef>

namespace CppRtos
{
	namespace Port
	{
		class ARMCM7
		{
		public:
			ARMCM7()
			{
			}

			~ARMCM7()
			{
			}

			inline bool isIRQMode( void ) const
			{
				std::uint32_t res = 0u;
				__asm volatile ("MRS %0, ipsr" : "=r" (res) );
				return( res  != 0u );
			}

			inline bool isIRQMasked( void )  const
			{
				return ( getPriorityMask() != 0u );
			}

			inline std::uint32_t getPriorityMask( void ) const
			{
				uint32_t res = 0u;
				__asm volatile ("MRS %0, primask" : "=r" (res) );
				return res;
			}


			inline std::uint32_t getIPSR( void )  const
			{
				std::uint32_t res = 0u;
				__asm volatile ("MRS %0, ipsr" : "=r" (res) );
				return (res);
			}

			inline std::uint32_t getAPSR( void ) const
			{
				std::uint32_t res = 0u;
				__asm volatile ("MRS %0, apsr" : "=r" (res) );
				return (res);
			}

			inline std::uint32_t  getXPSR( void )  const
			{
				std::uint32_t res = 0u;
				__asm volatile ("MRS %0, xpsr" : "=r" (res) );
				return (res);
			}

			inline std::uint32_t getPSP( void )  const
			{
				std::uint32_t res = 0u;
				__asm volatile ("MRS %0, psp"  : "=r" (res) );
				return (res);
			}

			inline void setPSP( std::uint32_t topStack ) const
			{
				__asm volatile ("MSR psp, %0" : : "r" (topStack) : );
			}

			inline std::uint32_t getMSP( void )  const
			{
				std::uint32_t res = 0u;
				__asm volatile ("MRS %0, msp" : "=r" (res) );
				return (res);
			}

			inline void setMSP( std::uint32_t topStack )
			{
				__asm volatile ("MSR msp, %0" : : "r" (topStack) : );
			}

		};
	}
}
