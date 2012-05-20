#include "VariableSet.hpp"

std::ostream& operator<<(std::ostream& out, const VariableSet& set)
{
	out << "[ ";
	for (std::set<s4>::const_iterator it = set._variables.begin(); it != set._variables.end(); ++it)
	{
		out << *it << " ";
	}
	out << "]";

	return out;
}
