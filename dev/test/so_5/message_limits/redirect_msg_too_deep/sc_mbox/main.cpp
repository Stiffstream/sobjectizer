/*
 * A simple test for message limits (redirecting message with
 * too deep overlimit reaction level).
 */

#include "../test_logic.inl"

int
main()
{
	do_test( "simple too deep message redirect on MPSC-mboxes test",
			 []( a_manager_t & m, a_worker_t & w ) {
		w.set_self_mbox( w.so_direct_mbox() );

		m.set_self_mbox( m.so_direct_mbox() );
		m.set_target_mbox( w.so_direct_mbox() );
	} );

	return 0;
}

