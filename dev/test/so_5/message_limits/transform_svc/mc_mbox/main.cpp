/*
 * A simple test for message limits (transforming the service request).
 */

#include "../test_logic.inl"

int
main()
{
	do_test( "simple test for transformation of service request (MPMC-mbox)",
		[]( a_test_t & a ) {
			a.set_working_mbox( a.so_environment().create_mbox() );
		} );

	return 0;
}

