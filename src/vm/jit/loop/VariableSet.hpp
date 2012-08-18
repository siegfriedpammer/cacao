#ifndef _VARIABLE_SET
#define _VARIABLE_SET

#include <set>
#include <iostream>

#include "vm/types.h"

/**
 * A container for variables names.
 * Every variable is contained no more than once.
 */
class VariableSet
{
	std::set<s4> _variables;

public:

	typedef std::set<s4>::iterator iterator;
	
	void insert(s4 variableIndex) { _variables.insert(variableIndex); }
	void remove(s4 variableIndex) { _variables.erase(variableIndex); }
	bool contains(s4 variableIndex) { return _variables.find(variableIndex) != _variables.end(); }

	std::set<s4>::iterator begin() { return _variables.begin(); }
	std::set<s4>::iterator end() { return _variables.end(); }

	friend std::ostream& operator<<(std::ostream&, const VariableSet&);
};

#endif
