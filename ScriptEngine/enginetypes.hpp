#pragma once

#include <string>

namespace NaReTi{

	/* types used by the script engine
	 */

	typedef void basicFunc(void);

	/* A function handle that can be used to let the script engine
	 * call the associated function.
	 */
	struct FunctionHandle
	{
		friend class ScriptEngine;

		FunctionHandle() = default;

		FunctionHandle(basicFunc* _ptr)
		{
			ptr = _ptr;
		}
		
		template< typename _T>
		FunctionHandle(_T* _ptr)
		{
			ptr = (void*)_ptr;
		}

		operator bool() const
		{
			return ptr != nullptr;
		}

	private:
		void* ptr;
	};

	/* Configuration options
	*/

	enum OptimizationLvl
	{
		None,
		Basic
	};

	struct Config
	{
		OptimizationLvl optimizationLvl;

		//path where the script files are found
		std::string scriptLocation;
	};
}