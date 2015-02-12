/*
 * A test for sequence of child cooperation deregistration.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

struct msg_child_started : public so_5::rt::signal_t {};

void
create_and_register_agent(
	so_5::rt::environment_t & env,
	const std::string & parent_coop_name,
	int ordinal,
	int max_children,
	int level,
	int max_deep );

class a_test_t : public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public :
		a_test_t(
			so_5::rt::environment_t & env,
			const std::string & self_name,
			const std::string & parent_coop_name,
			int max_children,
			int level,
			int max_deep )
			:	base_type_t( env )
			,	m_self_name( self_name )
			,	m_parent_coop_name( parent_coop_name )
			,	m_max_children( max_children )
			,	m_level( level )
			,	m_max_deep( max_deep )
			,	m_started_children( 0 )
			,	m_self_mbox( env.create_local_mbox( self_name ) )
		{
		}

		void
		so_define_agent()
		{
			so_subscribe( m_self_mbox )
				.event( &a_test_t::evt_child_started );
		}

		void
		so_evt_start()
		{
			if( m_level != m_max_deep )
			{
				for( int i = 0; i != m_max_children; ++i )
					create_and_register_agent(
							so_environment(),
							m_self_name,
							i,
							m_max_children,
							m_level + 1,
							m_max_deep );
			}
			else
				notify_parent();
		}

		void
		evt_child_started(
			const so_5::rt::event_data_t< msg_child_started > & )
		{
			m_started_children += 1;
			if( m_started_children == m_max_children )
			{
				if( m_level )
					notify_parent();
				else
					so_environment().deregister_coop(
							so_coop_name(),
							so_5::rt::dereg_reason::normal );
			}
		}

	private :
		const std::string m_self_name;
		const std::string m_parent_coop_name;

		const int m_max_children;

		const int m_level;

		const int m_max_deep;

		int m_started_children;

		so_5::rt::mbox_t m_self_mbox;

		void
		notify_parent()
		{
			so_environment().create_local_mbox( m_parent_coop_name )->
					deliver_signal< msg_child_started >();
		}
};

std::string
create_coop_name(
	const std::string & parent_coop_name, int level, int ordinal )
{
	std::ostringstream s;
	if( parent_coop_name.empty() )
		s << "a_" << level << "_" << ordinal;
	else
		s << parent_coop_name << "_" << level << "_" << ordinal;

	return s.str();
}

void
create_and_register_agent(
	so_5::rt::environment_t & env,
	const std::string & parent_coop_name,
	int ordinal,
	int max_children,
	int level,
	int max_deep )
{
	so_5::rt::agent_coop_unique_ptr_t coop = env.create_coop(
			create_coop_name( parent_coop_name, level, ordinal ) );
	if( level )
		coop->set_parent_coop_name( parent_coop_name );

	coop->add_agent(
			new a_test_t(
					env,
					coop->query_coop_name(),
					parent_coop_name,
					max_children,
					level,
					max_deep ) );

	env.register_coop( std::move( coop ) );
}

class test_coop_listener_t
	:	public so_5::rt::coop_listener_t
{
	public :
		test_coop_listener_t( int & active_coops_counter )
			:	m_active_coops( active_coops_counter )
		{}

		virtual void
		on_registered(
			so_5::rt::environment_t &,
			const std::string & )
		{
			++m_active_coops;
		}

		virtual void
		on_deregistered(
			so_5::rt::environment_t & env,
			const std::string & coop_name,
			const so_5::rt::coop_dereg_reason_t &)
		{
			--m_active_coops;

			if( "a_0_0" == coop_name )
				env.stop();
		}

		static so_5::rt::coop_listener_unique_ptr_t
		make( int & active_coops_counter )
		{
			return so_5::rt::coop_listener_unique_ptr_t(
					new test_coop_listener_t( active_coops_counter ) );
		}

	private :
		int & m_active_coops;
};

class test_env_t
{
	public :
		test_env_t()
			:	m_active_coops( 0 )
		{
		}

		void
		init( so_5::rt::environment_t & env )
		{
			create_and_register_agent( env, "", 0, 4, 0, 8 );
		}

		void
		check_result() const
		{
			if( 0 != m_active_coops )
				throw std::runtime_error(
						"There are some not deregistered cooperations" );
		}

		int &
		active_coops_counter()
		{
			return m_active_coops;
		}

	private :
		int m_active_coops;
};

int
main()
{
	try
	{
		test_env_t test_env;
		so_5::launch(
			[&]( so_5::rt::environment_t & env )
			{
				test_env.init( env );
			},
			[&]( so_5::rt::environment_params_t & params )
			{
				params.coop_listener(
						test_coop_listener_t::make(
								test_env.active_coops_counter() ) );
			} );

		test_env.check_result();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
