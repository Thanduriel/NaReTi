#pragma once

#include "module.hpp"

namespace par
{
	/*a collection of modules that  provides a single interface to search in every module
	 * it containts.
	 */
	class ModuleLibrary
	{
	public:
		//create a lib with a default module that cannot be removed
		ModuleLibrary(NaReTi::Module& _module)
		{
			addModule(_module);
		}

		void addModule(NaReTi::Module& _module) { m_modules.push_back(&_module); };
		
		//resets the library to only contain the first value that was given
		void reset() { m_modules.resize(1); };

		Type* getType(const std::string& _str);
		par::Function* getFunction(const std::string& _name,
			const std::vector<par::Parameter>::iterator& _begin,
			const std::vector<par::Parameter>::iterator& _end);

	private:
		std::vector < NaReTi::Module* > m_modules;
	};
}