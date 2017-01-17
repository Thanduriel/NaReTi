#include "enginetypes.hpp"
#include "module.hpp"
#include "moduleloader.hpp"
#include "threadpool.hpp"
#include <assert.h>

#pragma once

namespace codeGen{
	class Compiler;
	class Optimizer;
}
namespace par{
	class Parser;
	class GenericsParser;
}

namespace lang{
	struct BasicModule;
}


namespace NaReTi{

	class ScriptEngine
	{
	public:
		ScriptEngine(const Config& _config);
		~ScriptEngine();

		// @return the module with the given name
		// loads the module if not found
		Module* getModule(const std::string& _name);

		// compiles a given source file and adds the module
		// removes the previous content if there is any
		// @param _name is the file name without ending.
		// @param _dest when provided symbols are added to this module
		bool loadModule(NaReTi::Module& _module);
		bool loadModule(const std::string& _name);
		// @param _text the source code
		bool loadModule(NaReTi::Module& _module, const std::string& _text);

		// Removes a module with the given name and all it's symbols to release it's memory.
		// This should only be used if no other module depends on the module.
		// @param _keepBinary If true no compiled functions are removed
		bool unloadModule(const std::string& _moduleName, bool _keepBinary = false);

		//Creates a new and empty module to build extern.
		Module& createModule(const std::string& _moduleName);

		// returns a handle to the function with the given name if existent
		FunctionHandle getFuncHndl(const std::string& _name) const;

		// call without template params(just for convenience)
		void call(FunctionHandle _hndl) const;
		
		template< typename _Ret, typename... _Args>
		_Ret call(FunctionHandle _hndl, _Args... _args) const
		{
			assert((bool)_hndl);

			//magic typecasting to the given function type
			auto func = (_Ret (*) (_Args...))_hndl.ptr;
			//the call
			return func(_args...);
		}

		// the given call will be executed in an extra thread
		// when checkTime() is used an error is thrown when the _maxTime is exceeded
		template< typename... _Args>
		void callRestricted(FunctionHandle _hndl, float _maxTime, _Args... _args)
		{
			auto func = (void(*) (_Args...))_hndl.ptr;
			auto wrapper = [=]()
			{
				func(_args...);
			};

			m_tasks.emplace_back(m_threadPool.push(wrapper), _maxTime);
		}

		// checks whether any running task exceeds its _maxTime
		void checkTasks();

		// this is only for test purposes
		template< typename... _Args>
		std::shared_ptr<utils::ThreadPool::Task> testcall(FunctionHandle _hndl, _Args&&... _args)
		{
			auto func = (void(*) (_Args...))_hndl.ptr;
			auto wrapper = [=]()
			{
				func(_args...);
			};

			return m_threadPool.push(wrapper);

		}

		Config& config() { return m_config; }
	private:
		NaReTi::Module* findModule(const std::string& _name);
		// unloads the module including compiled data and leaves it empty in the same place
		void rebuildModule(NaReTi::Module* _module);

		Config m_config;
		ModuleLoader m_moduleLoader;

		std::string extractName(const std::string& _fullName) const;

		lang::BasicModule* m_basicModule;

		par::Parser* m_parser;
		par::GenericsParser* m_genericsParser;

		codeGen::Compiler* m_compiler;
		codeGen::Optimizer* m_optimizer;

		std::vector< FunctionHandle > m_nativeFunctions;
		std::vector< std::unique_ptr<Module> > m_modules;

		utils::ThreadPool m_threadPool;

		struct TaskRestriction
		{
			TaskRestriction(utils::ThreadPool::TaskHandle&& _task, float _maxTime):
				task(std::move(_task)),
				maxTime(_maxTime),
				begin(clock())
			{

			}
			utils::ThreadPool::TaskHandle task;
			float maxTime;
			clock_t begin;
		};
		std::vector<TaskRestriction> m_tasks;
	};

}
