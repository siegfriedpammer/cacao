#include "NumericInstruction.hpp"

s4 NumericInstruction::lowerBounds[3] =
{
	0,
	0,
	std::numeric_limits<s4>::min()
};

s4 NumericInstruction::upperBounds[3] =
{
	0,
	std::numeric_limits<s4>::max(),
	std::numeric_limits<s4>::max()
};
