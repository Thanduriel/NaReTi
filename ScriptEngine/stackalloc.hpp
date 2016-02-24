#pragma once

#include <vector>

namespace utils{
	template < size_t _BlockSize>
	class StackAllocator
	{
	public:
		StackAllocator() 
		{
			newBlock();
			bp = 0;
		}
		~StackAllocator()
		{
			for (auto block : memBlocks)
				free(block);
		}

		template< typename _T, typename... _Args>
		_T* construct(_Args&&... _args)
		{
			//check whether enough space is avialable
			if (sp - (char*)memBlocks[bp] + sizeof(_T) > _BlockSize) newBlock();

			//call constructor
			auto ptr = new(sp)_T(std::forward< _Args >(_args)...);
			sp += sizeof(_T);
			return ptr;
		}

		//throws away extra blocks and resets to the default state
		void reset()
		{
			for (size_t i = 1; i < memBlocks.size(); ++i)
				free(memBlocks[i]);

			memBlocks.resize(1);
		}
	private:
		void newBlock()
		{
			memBlocks.push_back(malloc(_BlockSize));
			bp++;
			sp = (char*)memBlocks.back();
		}

		char* sp; // stack pointer
		size_t bp; //block pointer
		std::vector< void* > memBlocks;
	};

	typedef utils::StackAllocator<1024> StackAlloc;
}