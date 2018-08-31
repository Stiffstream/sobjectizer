#pragma once

#include <so_5/h/declspec.hpp>

#include <vector>

#if defined(THIRD_PRJ)
	#define THIRD_FUNC SO_5_EXPORT
#else
	#define THIRD_FUNC SO_5_IMPORT
#endif

namespace third
{

using func_t = void (*)(void *);

using func_container_t = std::vector<func_t>;

THIRD_FUNC void run(const func_container_t & funcs);

} /* namespace third */

