#pragma once

namespace NaReTi{

	/* types used by the script engine
	 */

	typedef void basicFunc(void);

	//some kind of param and retval information for safety?
	struct FunctionHandle
	{
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

		void* ptr;
	};

}