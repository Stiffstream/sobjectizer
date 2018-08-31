#pragma once

#include <so_5/h/declspec.hpp>

#if defined(SECOND_PRJ)
	#define SECOND_FUNC SO_5_EXPORT
#else
	#define SECOND_FUNC SO_5_IMPORT
#endif

namespace second
{

SECOND_FUNC void make_coop(void * env);

} /* namespace second */

