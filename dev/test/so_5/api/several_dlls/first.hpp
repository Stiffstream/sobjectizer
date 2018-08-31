#pragma once

#include <so_5/h/declspec.hpp>

#if defined(FIRST_PRJ)
	#define FIRST_FUNC SO_5_EXPORT
#else
	#define FIRST_FUNC SO_5_IMPORT
#endif

namespace first
{

FIRST_FUNC void make_coop(void * env);

} /* namespace first */

