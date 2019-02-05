#include "first.hpp"
#include "second.hpp"
#include "third.hpp"

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

int main()
{
	run_with_time_limit( []{
			third::func_container_t funcs;
			funcs.push_back( &first::make_coop );
			funcs.push_back( &second::make_coop );

			third::run( funcs );
		},
		5 );
}

