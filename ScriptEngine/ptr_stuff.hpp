#pragma once

namespace utils{
	/* A helper that makes sure that the given pointer is
	 * reseted to nullptr when it goes out of scope.
	 */
	class PtrReset
	{
	public:
		PtrReset(){}
		template < typename _T>
		PtrReset(_T** _dst) : destination((void**)_dst){}
		template < typename _T>
		void setPtr(_T** _dst) { destination = (void**)_dst; }

		~PtrReset()
		{
			*destination = nullptr;
		}

	private:
		void** destination;
	};
}