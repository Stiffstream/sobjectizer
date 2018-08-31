#include "third.hpp"

#include <so_5/all.hpp>

namespace third
{

THIRD_FUNC void run(const func_container_t & funcs)
{
	try
	{
		so_5::launch( [&](so_5::environment_t & env) {
			for( auto f : funcs ) {
				(*f)( &env );
			}
		},
		[](so_5::environment_params_t & params) {
			params.message_delivery_tracer(
					so_5::msg_tracing::std_cout_tracer() );
		} );
	}
	catch( const std::exception & x )
	{
		std::cout << "Exception caught: " << x.what() << std::endl;
		std::abort();
	}
}

} /* namespace third */

