/*
 * A test for so_5::details::lock_holder_detector.
 */

#include <so_5/details/h/sync_helpers.hpp>

#include <various_helpers_1/ensure.hpp>

#include <iostream>

template< typename LOCK_TYPE >
class test : private ::so_5::details::lock_holder_detector<LOCK_TYPE>::type
	{
		int i = 0;
	public :
		void inc() {
			this->lock_and_perform( [&]{ ++i; } );
		}

		int val() const {
			return this->lock_and_perform( [&]{ return i; } );
		}
	};

int
main()
{
	static_assert( sizeof(std::mutex) > sizeof(so_5::null_mutex_t),
			"sizeof of std::mutex is expected to be sizeof of so_5::null_mutex_t" );

	test< std::mutex > t_real_mutex;
	test< so_5::null_mutex_t > t_null_mutex;

	ensure_or_die( sizeof(t_real_mutex) > sizeof(t_null_mutex),
			"sizeof of t_real_mutex is expected to be sizeof of t_null_mutex" );

	return 0;
}

