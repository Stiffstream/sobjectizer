/*
	SObjectizer 5.
*/

#include <so_5/rt/h/so_layer.hpp>

#include <so_5/h/exception.hpp>

namespace so_5
{

void
layer_t::start()
{
}

void
layer_t::shutdown()
{
}

void
layer_t::wait()
{
}

environment_t &
layer_t::so_environment()
{
	if( nullptr == m_env )
	{
		throw so_5::exception_t(
			"so_environment isn't bound to this layer",
			rc_layer_not_binded_to_so_env );
	}

	return *m_env;
}

void
layer_t::bind_to_environment( environment_t * env )
{
	m_env = env;
}

} /* namespace so_5 */

