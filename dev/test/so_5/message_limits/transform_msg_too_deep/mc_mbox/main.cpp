/*
 * A simple test for message limits (transforming message with
 * too deep overlimit reaction level).
 */

#include "../test_logic.inl"

int
main()
{
	do_test( "simple too deep message transformations on MPMC-mboxes test",
		[]( a_manager_t & m, a_worker_t & w ) {
			auto m_mbox = m.so_environment().create_mbox();
			auto w_mbox = m.so_environment().create_mbox();

			w.set_self_mbox( w_mbox );

			m.set_self_mbox( m_mbox );
			m.set_target_mbox( w_mbox );
		} );

	return 0;
}

