#pragma once

#include "symbols.hpp"
#include <unordered_map>

namespace par{
	/*the plan
	 * when a template param is specified a genericType is created
	 * as well as types with the corresponding name
	 * code is parsed regularly
	 * on specialization the tree and vars are copied and the param types substituted
	 * linking takes place
	 * the new code can be added to the current module
	 */
	class GenericType : public ComplexType
	{
	public:
		GenericType(const std::string& _name, const std::vector<std::string>& _params);

		void addTypeParam(ComplexType& _type) { m_typeParams.emplace_back(&_type); }

		size_t getParamCount() const { return m_typeParams.size(); }

		ComplexType* getSpecialization(ComplexType* _params);
		std::string mangledName(ComplexType* _params);


		/* creates a specialized type in the given module with the given param types
		 * Does not perform a test whether the supplied amount is correct,
		 * use getParamCount() before to ensure this.
		 */
		ComplexType* makeSpecialisation(const std::string& _name, ComplexType* _params);
	private:
		std::vector< std::unique_ptr<ComplexType> > m_typeParams; // generic params 
	};
}