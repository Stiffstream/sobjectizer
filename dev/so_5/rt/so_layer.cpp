/*
	SObjectizer 5.
*/

#include <so_5/h/exception.hpp>

#include <so_5/rt/h/so_layer.hpp>

namespace so_5
{

namespace rt
{

so_layer_t::so_layer_t()
{
}

so_layer_t::~so_layer_t()
{
}

void
so_layer_t::start()
{
}

void
so_layer_t::shutdown()
{
}

void
so_layer_t::wait()
{
}

environment_t &
so_layer_t::so_environment()
{
	if( nullptr == m_so_environment )
	{
		throw so_5::exception_t(
			"so_environment isn't bound to this layer",
			rc_layer_not_binded_to_so_env );
	}

	return *m_so_environment;
}

void
so_layer_t::bind_to_environment( environment_t * env )
{
	m_so_environment = env;
}

} /* namespace rt */

} /* namespace so_5 */

