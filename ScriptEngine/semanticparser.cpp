#include "semanticparser.hpp"
#include "atomics.hpp"
#include <string>
#include <iostream>
#include "parexception.hpp"
#include "generics.hpp"

using namespace std;

namespace par
{
	SemanticParser::SemanticParser() :
		m_moduleLib(*lang::g_module),
		m_typeInfo(lang::g_module->getBasicType(BasicType::Void))
	{
		m_arrayTypeGen.buildConst(lang::g_module->getBasicType(BasicType::Void));
	}

	// ************************************************** //

	void SemanticParser::setModule(NaReTi::Module& _module)
	{
		m_currentModule = &_module; 
		resetScope();
		m_allocator = &_module.getAllocator();
		m_moduleLib.reset(); 
		for (auto& dep : _module.m_dependencies)
			m_moduleLib.addModule(*dep);

		//the current module is a valid resource as well
		m_moduleLib.addModule(_module);
	}

	// ************************************************** //

	void SemanticParser::linkCall(ASTCall& _node)
	{
		for (auto& arg : _node.args)
		{
			//only not linked functions have no typeinfo at this point
			if (arg->typeInfo == nullptr && (arg->type == ASTType::Call || arg->type == ASTType::Member))
			{
				ASTCall& call = *(ASTCall*)arg;
				linkCall(call);
			}
		}

		if (_node.type == ASTType::Member)
		{
			ASTMember& memberNode = *(ASTMember*)&_node;
			linkMember(memberNode);
			return;
		}

		for (auto& arg : _node.args) 
			if (arg->typeInfo == nullptr && arg->type == ASTType::String) 
			{ 
				throw ParsingError("Unknown Symbol: " + ((ASTUnlinkedSym*)arg)->name);
			}

		m_funcQuery.clear();
		if (_node.name == "free")
			int brk = 1234;

		Function* func = m_moduleLib.getFunction(_node.name, _node.args.begin(), _node.args.end(), m_funcQuery);
		if (!func)
		{
			bool match = false;
			//try the match with the fewest differences first
			std::sort(m_funcQuery.begin(), m_funcQuery.end());
			for (auto& funcMatch : m_funcQuery)
			{
				if (tryArgCasts(_node, *funcMatch.function))
				{
					func = funcMatch.function;
					match = true;
					break;
				}
			}
			// no match possible
			if (!match)
			{
				//construct a fancy error message
				string args;
				for (auto& arg : _node.args)
				{
					args += buildTypeInfoString(*arg->typeInfo) + ',';
				}
				args.resize(args.size() - 1); //remove final comma; a function without args should not land here
				throw ParsingError("No function with the given signature found: " + _node.name + '(' + args + ')');
			}
		}
		_node.function = func;
		_node.typeInfo = &func->returnTypeInfo;
	}

	// ************************************************** //

	void SemanticParser::linkMember(ASTMember& _node)
	{
		//same error as in linkCall
		if (_node.args[0]->type == ASTType::String) 
			throw ParsingError("Unknown Symbol: " + ((ASTUnlinkedSym*)_node.args[0])->name);

		ASTUnlinkedSym* strLeaf = (ASTUnlinkedSym*)_node.args[1];
		size_t i = 0;
		// check members of the type on top of the stack
		for (auto& member : _node.args[0]->typeInfo->type.scope.m_variables)
		{
			if (member->name == strLeaf->name)
			{
				_node.typeInfo = &member->typeInfo;
				_node.index = i;
				_node.instance = _node.args[0];
				return;
			}
			i++;
		}

		throw ParsingError(_node.args[0]->typeInfo->type.name + " has no member called \"" + strLeaf->name + "\"");
	}

	// ************************************************** //

	bool SemanticParser::tryArgCasts(ASTCall& _node, Function& _func)
	{
		std::vector<Function*> casts; casts.resize(_node.args.size());
		ZeroMemory(&casts[0], sizeof(Function*) * casts.size()); //make sure to only have nullptr; maybe not std conform

		for (int i = 0; i < (int)_node.args.size(); ++i)
		{
			TypeInfo& t0 = *_node.args[i]->typeInfo;
			TypeInfo& t1 = _func.scope.m_variables[i]->typeInfo;
			if (t0 == t1) continue;
			//right site of an assignment may not be casted
			if (_func.intrinsicType == Function::Assignment && i == 0) return false;

			casts[i] = typeCast(t0, t1);
			if (!casts[i]) return false;
		}

		//put casts into the tree
		for (int i = 0; i < (int)_node.args.size(); ++i)
		{
			if (!casts[i]) continue;

			ASTCall& call = *m_allocator->construct<ASTCall>();
			call.function = casts[i];
			call.args.push_back(_node.args[i]);
			call.typeInfo = &casts[i]->returnTypeInfo;
			_node.args[i] = &call;
		}

		return true;
	}

	// ************************************************** //

	Function* SemanticParser::typeCast(TypeInfo& _t0, TypeInfo& _t1)
	{
		for (auto& cast : _t0.type.typeCasts)
		{
			if (cast->returnTypeInfo == _t1)
			{
				return cast.get();
			}
		}

		return nullptr;
	}

	// ************************************************** //

	std::string SemanticParser::buildTypeInfoString(const TypeInfo& _t)
	{
		return _t.type.name + (_t.isConst ? " const" : "")
			+ (_t.isArray ? "[]" : "")
			+ (_t.isReference ? "&" : "");
	}

	// ************************************************** //

	ASTExpNode* SemanticParser::popCondNode()
	{
		ASTExpNode* node = popNode();

		if (node->typeInfo->type.basic != BasicType::FlagBool)
		{
			Function* cast = typeCast(*node->typeInfo, TypeInfo(lang::g_module->getBasicType(BasicType::FlagBool)));

			if (!cast) throw ParsingError("Can not interpret " + node->typeInfo->type.name + " as bool.");

			ASTCall& call = *m_allocator->construct<ASTCall>();
			call.function = cast;
			call.args.push_back(node);
			
			return &call;
		}
		return node;
	}

	// ************************************************** //
	// boost parser declarations						  //
	// ************************************************** //

	using namespace boost::fusion;

	std::string fa(boost::fusion::vector2<char, std::vector<char>>& _attr)
	{
		std::string str;
		str.resize(_attr.m1.size() + 1);
		str[0] = _attr.m0;
		for (int i = 0; i < _attr.m1.size(); ++i)
			str[i + 1] = _attr.m1[i];

		//	std::cout << str << std::endl;

		return str;
	}
	void SemanticParser::varDeclaration(string&  _attr)
	{
		auto typeInfo = buildTypeInfo();
		m_currentScope->m_variables.emplace_back(m_allocator->construct<VarSymbol>(_attr, typeInfo));
	}

	void SemanticParser::pushLatestVar()
	{
		pushSymbol(m_currentScope->m_variables.back()->name);
	}

	// ************************************************** //

	void SemanticParser::typeDeclaration(std::string& _attr)
	{
		if (m_genericTypeParams.size())
		{
			g_genericsParser->setModule(m_currentModule);
			std::vector< string > realNames; 
			realNames.reserve(m_genericTypeParams.size());
			for (int i = 0; i < m_genericTypeParams.size(); ++i)
			{
				//since these are auto generated they should always be found
				auto& alias = *m_currentModule->getTypeAlias("__T_" + std::to_string(i));
				alias.first = m_genericTypeParams[i];
				realNames.push_back(alias.second->name);
			}
			string name = g_genericsParser->mangledName(_attr, realNames);
			m_currentModule->m_types.emplace_back(new ComplexType(name));
			m_genericTypeParams.clear();
		}
		else
		{
			m_currentModule->m_types.emplace_back(new ComplexType(_attr));
		}
		m_currentScope = &m_currentModule->m_types.back()->scope;
	}

	// ************************************************** //

	void SemanticParser::genericTypePar(std::string& _attr)
	{
		m_genericTypeParams.push_back(_attr);
	}

	// ************************************************** //

	void SemanticParser::finishTypeDec()
	{
		ComplexType& type = *m_currentModule->m_types.back();

		m_typeDefaultGen.buildDefaultAssignment(type, *m_currentModule, m_moduleLib);
		m_typeDefaultGen.buildElemAccess(type, *m_currentModule);
		m_typeDefaultGen.buildRefAssignment(type, *m_currentModule);
		m_typeDefaultGen.buildVoidCast(type, *m_currentModule);
	}

	// ************************************************** //

	void SemanticParser::funcDeclaration(std::string & _attr)
	{
		m_currentModule->m_functions.emplace_back(new Function(_attr, buildTypeInfo()));

		m_currentFunction = m_currentModule->m_functions.back();

		if (m_currentFunction->returnTypeInfo.type.basic == BasicType::Complex)
		{
			Function& func = *m_currentModule->m_functions.back();
			func.scope.m_variables.emplace_back(m_allocator->construct<VarSymbol>("", m_currentFunction->returnTypeInfo));
			func.scope.m_variables.back()->typeInfo.isReference = true; // it is the pointer to the stack var
			func.bHiddenParam = true;
		}

		//init environment
		m_targetScope = &m_currentModule->m_functions.back()->scope;
		m_targetScope->m_parent = m_currentScope;
		//param dec is outside of the following code scope
		m_currentScope = m_targetScope;
	}

	void SemanticParser::finishParamList()
	{
		Function& function = *m_currentModule->m_functions.back();
		function.paramCount = (int)function.scope.m_variables.size();
	}

	// ************************************************** //

	void SemanticParser::finishGeneralExpression()
	{
		while (m_stack.size())
			m_currentCode->push_back(popNode());
	}

	// ************************************************** //

	void SemanticParser::beginCodeScope()
	{
		ASTCode* codeNode;
		if (m_targetScope)
		{
			codeNode = m_targetScope;
		}
		else
		{
			codeNode = m_allocator->construct<ASTCode>();
			//since the node is linked to no other node it is placed into the currently open scope
			m_currentCode->push_back(codeNode);
		}
		m_targetScope = nullptr;
		codeNode->parent = m_currentCode;
		codeNode->m_parent = m_currentCode;
		m_currentCode = codeNode;
		m_currentScope = codeNode;
	}

	// ************************************************** //

	void SemanticParser::finishCodeScope()
	{
		//return to the previous scope
		m_currentCode = m_currentCode->parent;
		m_currentScope = m_currentScope->m_parent;
	}

	// ************************************************** //

	void SemanticParser::ifConditional()
	{
		ASTBranch& branchNode = *m_allocator->construct<ASTBranch>();
		branchNode.condition = popCondNode();
		branchNode.ifBody = m_allocator->construct<ASTCode>();
		m_targetScope = branchNode.ifBody;

		m_currentCode->push_back(&branchNode);
	}

	// ************************************************** //

	void SemanticParser::elseConditional()
	{
		ASTBranch& parentNode = *(ASTBranch*)m_currentCode->back();
		parentNode.elseBody = m_allocator->construct<ASTCode>();
		m_targetScope = parentNode.elseBody;
		beginCodeScope();
	}

	// ************************************************** //

	void SemanticParser::returnStatement()
	{
		m_currentCode->emplace_back(m_allocator->construct<ASTReturn>());
		ASTReturn& retNode = *(ASTReturn*)m_currentCode->back();

		if (m_currentFunction->returnTypeInfo.type.basic != BasicType::Void)
		{
			retNode.body = popNode();
			assert(retNode.body->typeInfo);

			//types do not match
			//todo: make typechecking same for functions and return
			if (*retNode.body->typeInfo != m_currentFunction->returnTypeInfo)
			{
				auto cast = typeCast(*retNode.body->typeInfo, m_currentFunction->returnTypeInfo);
				if (!cast) throw ParsingError("Type mismatch found \"" 
					+ buildTypeInfoString(*retNode.body->typeInfo)
					+ "\" but expected \""
					+ buildTypeInfoString(m_currentFunction->returnTypeInfo) + "\"");
				ASTCall& call = *m_allocator->construct<ASTCall>();
				call.function = cast;
				call.args.push_back(retNode.body);
				call.typeInfo = &cast->returnTypeInfo;
				retNode.body = &call;
			}
		}
	}

	// ************************************************** //

	void SemanticParser::loop()
	{
		ASTLoop& node = *m_allocator->construct<ASTLoop>();
		node.condition = popCondNode();
		node.body = m_allocator->construct<ASTCode>();
		m_targetScope = node.body;

		m_currentCode->push_back(&node);
	}

	// ************************************************** //

	void SemanticParser::term(const string& _operator)
	{
		//build new node
		ASTCall* astNode = _operator == "." ? m_allocator->construct<ASTMember>() : m_allocator->construct<ASTCall>();
		astNode->name = _operator;
		astNode->typeInfo = nullptr;

		//pop the used params
		astNode->args.resize(2);
		//no popNode since the tree is not final
		astNode->args[1] = m_stack.back(); m_stack.pop_back();
		//traverse the tree to find the right position in regard of precedence
		ASTExpNode** dest = findPrecPos(&m_stack.back(), *astNode);
		astNode->args[0] = *dest;
		*dest = astNode; // put this node there

//		cout << _operator << endl;
	}

	// ************************************************** //

	void SemanticParser::unaryTerm(const boost::optional<std::string>& _str)
	{
		if (!_str.is_initialized()) return;

		ASTExpNode* arg = m_stack.back();
		
		if (arg->type == ASTType::LeafInt)
		{
			ASTLeafInt& leaf = *(ASTLeafInt*)arg;
			leaf.value = -leaf.value;
			return;
		}
		else if (arg->type == ASTType::LeafFloat)
		{
			ASTLeafFloat& leaf = *(ASTLeafFloat*)arg;
			leaf.value = -leaf.value;
			return;
		}

		//only take the arg of the stack when a new one is going to be added
		m_stack.pop_back();

		ASTCall* astNode = m_allocator->construct<ASTCall>();
		astNode->name = _str.value();
		astNode->typeInfo = nullptr;

		astNode->args.resize(1);
		astNode->args[0] = arg;
		m_stack.push_back(astNode);
	}

	// ************************************************** //

	void SemanticParser::call(string& _name)
	{
		//build new node
		ASTCall* node = m_allocator->construct<ASTCall>();
		node->name = _name;
		node->typeInfo = nullptr;

		m_stack.push_back(node);
	}

	void SemanticParser::argSeperator()
	{
		ASTExpNode* expNode = m_stack.back(); m_stack.pop_back();
		ASTCall* callNode = (ASTCall*)m_stack.back();
		callNode->args.push_back(expNode);
	}

	// ************************************************** //

	void SemanticParser::pushSymbol(string& _name)
	{
		VarSymbol* var;
		//look in global scope
		var = m_moduleLib.getGlobalVar(_name);
		if (var)
		{
			bool isImported = false;
			for (auto& impVar : m_currentCode->m_importedVars) if (impVar == var){ isImported = true; break; }
			if (!isImported)
			{ 
				m_currentCode->m_importedVars.push_back(var); 
				//globals have to be pointers, this is the first point where we can make sure that isRef is set
				var->typeInfo.isReference = true; 
			}
		}
		else var = m_currentCode->getVar(_name);

		if (!var) //could still be a member
		{
			m_stack.push_back(m_allocator->construct<ASTUnlinkedSym>(_name));
			m_stack.back()->typeInfo = nullptr;
			return;
		}
		
		ASTLeafSym* leaf = m_allocator->construct<ASTLeafSym>(var);
		leaf->typeInfo = &var->typeInfo;
		m_stack.push_back(leaf);
//		cout << _name << endl;
	}

	void SemanticParser::pushFloat(double _val)
	{
		ASTLeafFloat* leaf = m_allocator->construct<ASTLeafFloat>((float)_val);
		leaf->typeInfo = m_allocator->constructUnsafe<TypeInfo>(lang::g_module->getBasicType(BasicType::Float));
		m_stack.push_back(leaf);
//		cout << _val << endl;
	}

	void SemanticParser::pushInt(int _val)
	{
		ASTLeafInt* leaf = m_allocator->construct<ASTLeafInt>(_val);
		leaf->typeInfo = m_allocator->constructUnsafe<TypeInfo>(lang::g_module->getBasicType(BasicType::Int));
		m_stack.push_back(leaf);
//		cout << _val << endl;
	}

	void SemanticParser::pushString(std::string& _str)
	{
		ASTUnlinkedSym* leaf = m_allocator->construct<ASTUnlinkedSym>(_str);
		leaf->typeInfo = m_allocator->constructUnsafe<TypeInfo>(lang::g_module->getBasicType(BasicType::String));
		leaf->typeInfo->isReference = true; // actually false, but strings can not be used as value currently
		m_stack.push_back(leaf);
	}

	// ************************************************** //

	void SemanticParser::lockLatestNode()
	{
		((ASTCall*)m_stack.back())->isLocked = true;
	}

	// ************************************************** //

	ASTExpNode** SemanticParser::findPrecPos(ASTExpNode** _tree, ASTCall& _node)
	{
		switch ((*_tree)->type)
		{
		case ASTType::Member:
		case ASTType::Call:
		{
			ASTCall& call = *((ASTCall*)(*_tree));
			if (!call.isLocked && lang::g_module->getPrecedence(call.name) > lang::g_module->getPrecedence(_node.name) && call.args.size() == 2) // only enter bin ops (unarys should have higher precedence)
			{
				return findPrecPos((ASTExpNode**)&call.args[1], _node);
			}
			break;
		}
		case ASTType::LeafInt:
		case ASTType::LeafFloat:
		case ASTType::LeafSym:
		case ASTType::Leaf:
			return _tree;
		}
		return _tree;
	}

	// ************************************************** //

	ComplexType& SemanticParser::getType(const std::string& _name)
	{
		ComplexType* type = m_moduleLib.getType(_name);
		if (!type) throw ParsingError("Unknown type: " + _name);

		return *type;
	}

	// ************************************************** //

	TypeInfo SemanticParser::buildTypeInfo()
	{
		if (m_genericTypeParams.size())
		{
			g_genericsParser->setModule(m_currentModule);

			ComplexType* params[8];
			for (int i = 0; i < (int)m_genericTypeParams.size(); ++i)
			{
				params[i] = &getType(m_genericTypeParams[i]);
				m_genericTypeParams[i] = params[i]->name; // aliases do not have the correct type
			}

			m_typeName = g_genericsParser->mangledName(m_typeName, m_genericTypeParams);

			if (!m_currentModule->getType(m_typeName))
				g_genericsParser->parseType("array", m_genericTypeParams.size(), params[0]);
			m_genericTypeParams.clear(); //clear afterwards
		}

		ComplexType& type = getType(m_typeName);

		return TypeInfo(type, m_typeInfo.isReference, m_typeInfo.isConst, m_typeInfo.isArray);
	}

	// ************************************************** //

	void SemanticParser::makeArray()
	 { 
		 //translate T[] to array<T>
		 m_genericTypeParams.push_back(m_typeName);
		 m_typeName = "array";
	//	 newTypeInfo(type.name);
	}

	// ************************************************** //

	void testFunc()
	{
		std::cout << "test got called!" <<  std::endl;
	}

	void testFuncStr(string& str)
	{
		std::cout << str << std::endl;
	}
}