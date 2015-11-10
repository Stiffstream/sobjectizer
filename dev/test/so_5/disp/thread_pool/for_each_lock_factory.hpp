#pragma once

#include <so_5/disp/thread_pool/h/pub.hpp>

#include <iostream>

template< typename L >
void
run_with_lock_factory(
	const char * factory_name,
	so_5::disp::thread_pool::queue_traits::lock_factory_t factory,
	L && action )
	{
		std::cout << "=== " << factory_name << " ===" << std::endl;
		action( factory );
		std::cout << "=======" << std::endl;
	}

template< typename L >
void
for_each_lock_factory( L && action )
	{
		using namespace so_5::disp::thread_pool::queue_traits;
		run_with_lock_factory( "combined_lock()", combined_lock_factory(),
				std::forward<L>(action) );

		run_with_lock_factory( "combined_lock(250us)",
				combined_lock_factory( std::chrono::microseconds(250) ),
				std::forward<L>(action) );

		run_with_lock_factory( "simple_lock",
				simple_lock_factory(),
				std::forward<L>(action) );
	}

