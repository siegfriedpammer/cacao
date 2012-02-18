#include <cstring>
#include <sstream>

#include "loop.hpp"
#include "toolbox/logging.hpp"
#include "vm/method.hpp"

namespace
{
	bool initialized = false;

	utf* classHello;
	utf* methodFill;
	utf* methodPrint;

	void init()
	{
		initialized = true;

		classHello = utf_new_char("Hello");
		methodFill = utf_new_char("fill");
		methodPrint = utf_new_char("print");
	}

	bool equals(utf* a, utf* b)
	{
		if (a->blength != b->blength)
			return false;

		return std::strncmp(a->text, b->text, a->blength) == 0;
	}
}

void loopFoo(jitdata* jd)
{
	if (!initialized)
		init();

	if (equals(jd->m->clazz->name, classHello)
		&& (equals(jd->m->name, methodFill) || equals(jd->m->name, methodPrint)))
	{
		log_message_utf("method: ", jd->m->name);

		for (s4 i = 0; i < jd->instructioncount; i++)
		{
			if (jd->instructions[i].opc == ICMD_IASTORE
				|| jd->instructions[i].opc == ICMD_IALOAD)
			{
				std::stringstream str;
				str << "line: " << jd->instructions[i].line;
				log_text(str.str().c_str());
				
				// remove check
				jd->instructions[i].flags.bits &= ~INS_FLAG_CHECK;

				if (INSTRUCTION_MUST_CHECK(&(jd->instructions[i])))
					log_text("check!");
				else
					log_text("don't check!");
			}
		}
	}
}
