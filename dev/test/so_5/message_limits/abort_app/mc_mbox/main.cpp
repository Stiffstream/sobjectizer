/*
 * A simple test for message limits (aborting application).
 */

#include "../test_logic.inl"

int
main()
{
	do_test( "mpmc_mbox test case",
		[]( a_test_t & a ) {
			a.set_working_mbox( a.so_environment().create_mbox() );
		} );

	return 0;
}

