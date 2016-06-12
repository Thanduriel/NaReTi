/* include this header only in source files
 * that define intrinsics.
 * The defined macros only work inside the function of a NaReTi::Module
 */

#define BASICASSIGN(Name, T, Instr) m_functions.emplace_back(new Function( m_allocator, Name, *m_types[ T ], Instr)); m_functions.back()->scope.m_variables[0]->typeInfo.isConst = false; m_functions.back()->intrinsicType = Function::Assignment;
#define BASICOPERATION(X, Y, Z) m_functions.emplace_back(new Function( m_allocator, X, *m_types[ Y ], Z));
#define BASICOPERATIONEXT(Name, InstrList, T0, T1, T2) m_functions.emplace_back(new Function( m_allocator, Name, InstrList, *m_types[ T0 ], m_types[ T1 ], m_types[ T2 ]));
#define BASICCAST(Instr, T0, T1) T0.type.typeCasts.emplace_back(new Function( m_allocator, "", Instr, T1, T0));
#define UNARYOPERATION(Name, T, Instr) m_functions.emplace_back(new Function( m_allocator, Name, Instr, T, T));

