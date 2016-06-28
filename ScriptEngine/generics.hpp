#pragma once

#include "ast.hpp"
#include <unordered_map>
#include "parser.hpp"
#include "moduleloader.hpp"

namespace par{

	class GenericsParser : public BasicParser
	{
	public:
		GenericsParser(NaReTi::ModuleLoader& _loader) : m_targetModule(nullptr), m_loader(_loader){}

		//returns the mangled name a type with the given name and arguments would have
		std::string mangledName(const std::string& _name, const std::vector<std::string>& _args );
		void parseType(const std::string& _name, int _argCount, ComplexType* _args);

		//todo: generics remain in their own modules managed by the GenericsParser
		void setModule(NaReTi::Module* _module) { m_targetModule = _module; }
	private:
		std::vector< NaReTi::Module* > m_specialisations;

		NaReTi::Module* m_targetModule;
		NaReTi::ModuleLoader& m_loader;
	};

	//owned by the scriptEngine
	extern GenericsParser* g_genericsParser;

	class GenericTrait
	{
	public:
		GenericTrait(const std::vector<std::string>& _params);
	protected:
		// generic params
		// the index is encoded in ComplexType::size
		std::vector< std::unique_ptr<ComplexType> > m_typeParams;
	};

	/*the plan
	 * when a template param is specified a genericType is created
	 * as well as types with the corresponding name
	 * code is parsed regularly
	 * on specialization the tree and vars are copied and the param types substituted
	 * linking takes place
	 * the new code can be added to the current module
	 */
	class GenericType : public ComplexType, public GenericTrait
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
	};



	class GenericFunction : public Function, public GenericTrait
	{
	public:
		GenericFunction(const std::string& _name, TypeInfo& _info, const std::vector<std::string>& _params);

		Function* makeSpecialisation(const std::string& _name, ComplexType* _params);
	private:
		//make a deep copy of an ast
		ASTNode* copyTree(ASTNode* _node, utils::DetorAlloc& _alloc);
	};
}