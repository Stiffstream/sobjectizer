/*
 * A simple test for message limits (redirecting service request with
 * too deep redirection level).
 */

#include "../test_logic.inl"

int
main()
{
	do_test( "simple too deep service request redirect on MPMC-mboxes test",
		[]( a_manager_t & m, a_worker_t & w ) {
			auto m_mbox = m.so_environment().create_mbox();
			auto w_mbox = m.so_environment().create_mbox();

			w.set_self_mbox( w_mbox );

			m.set_self_mbox( m_mbox );
			m.set_target_mbox( w_mbox );
		} );

	return 0;
}

