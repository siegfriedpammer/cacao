#ifndef _VARIABLE_SET
#define _VARIABLE_SET

#include <set>
#include <iostream>

#include <vm/types.h>

/**
 * A container for variables names.
 * Every variable is contained no more than once.
 */
class VariableSet
{
	std::set<s4> _variables;

public:
	
	void insert(s4 variableIndex) { _variables.insert(variableIndex); }

	friend std::ostream& operator<<(std::ostream&, const VariableSet&);
};

#endif
