/*
 * A test for so_5::null_mutex_t and std::shared_lock.
 */

#include <so_5/details/sync_helpers.hpp>

#include <shared_mutex>
#include <iostream>
#include <type_traits>

template< typename Mutex >
void
lock_then_unlock_shared( Mutex && mtx )
	{
		std::shared_lock< std::decay_t<Mutex> > lock{ mtx };
		std::cout << "lock_then_unlock_shared" << std::endl;
	}

int
main()
{
	so_5::null_mutex_t mtx;
	lock_then_unlock_shared( mtx );

	return 0;
}

