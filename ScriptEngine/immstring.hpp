#pragma once

namespace utils{

//#pragma pack
	template<typename T>
	class ImmBasicString
	{
	public:
		ImmBasicString(utils::DetorAlloc& _alloc, const std::string& _str) :
			size((int)_str.size())
		{
			
			buf = static_cast<T*>(_alloc.alloc((size+1) * sizeof(T)));
			memcpy(buf, _str.c_str(), (size+1) * sizeof(T));
		}

		T* buf;
		int size;
	};

	typedef ImmBasicString<char> ImmString;
}