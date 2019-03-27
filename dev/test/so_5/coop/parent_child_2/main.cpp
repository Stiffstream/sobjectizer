/*
 * A test for sequence of child cooperation deregistration.
 */

#include <iostream>
#include <sstream>
#include <set>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_child_started : public so_5::signal_t {};

void
create_and_register_agent(
	const so_5::coop_handle_t & parent,
	so_5::environment_t & env,
	const std::string & parent_coop_name,
	int ordinal,
	int max_children,
	int level,
	int max_deep );

class a_test_t : public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public :
		a_test_t(
			so_5::environment_t & env,
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
			,	m_self_mbox( env.create_mbox( self_name ) )
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
							so_coop(),
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
			mhood_t< msg_child_started > )
		{
			m_started_children += 1;
			if( m_started_children == m_max_children )
			{
				if( m_level )
					notify_parent();
				else
					so_deregister_agent_coop_normally();
			}
		}

	private :
		const std::string m_self_name;
		const std::string m_parent_coop_name;

		const int m_max_children;

		const int m_level;

		const int m_max_deep;

		int m_started_children;

		so_5::mbox_t m_self_mbox;

		void
		notify_parent()
		{
			so_5::send< msg_child_started >(
					so_environment().create_mbox( m_parent_coop_name ) );
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
	const so_5::coop_handle_t & parent,
	so_5::environment_t & env,
	const std::string & parent_coop_name,
	int ordinal,
	int max_children,
	int level,
	int max_deep )
{
	auto coop = env.make_coop( parent );

	coop->make_agent< a_test_t >(
			create_coop_name( parent_coop_name, level, ordinal ),
			parent_coop_name,
			max_children,
			level,
			max_deep );

	env.register_coop( std::move( coop ) );
}

class test_coop_listener_t
	:	public so_5::coop_listener_t
{
	public :
		test_coop_listener_t( std::set< so_5::coop_id_t > & ids )
			:	m_ids( ids )
		{}

		void
		on_registered(
			so_5::environment_t &,
			const so_5::coop_handle_t & coop ) noexcept override
		{
			std::lock_guard< std::mutex > lock{ m_lock };

			m_ids.insert( coop.id() );
		}

		void
		on_deregistered(
			so_5::environment_t & env,
			const so_5::coop_handle_t & coop,
			const so_5::coop_dereg_reason_t &) noexcept override
		{
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				m_ids.erase( coop.id() );
			}

			if( 2u == coop.id() )
				env.stop();
		}

		static so_5::coop_listener_unique_ptr_t
		make( std::set< so_5::coop_id_t > & names )
		{
			return so_5::coop_listener_unique_ptr_t(
					new test_coop_listener_t( names ) );
		}

	private :
		std::mutex m_lock;
		std::set< so_5::coop_id_t > & m_ids;
};

int
main()
{
	try
	{
		std::set< so_5::coop_id_t > ids;

		run_with_time_limit( [&] {
			so_5::launch(
				[&]( so_5::environment_t & env )
				{
					create_and_register_agent(
							so_5::coop_handle_t{}, env, "", 0, 4, 0, 8 );
				},
				[&]( so_5::environment_params_t & params )
				{
					params.disable_autoshutdown();
					params.coop_listener( test_coop_listener_t::make( ids ) );
				} );
			},
			240 );

		if( !ids.empty() )
		{
			for( const auto & n : ids )
				std::cout << "unregistered coop: '" << n << "'" << std::endl;

			throw std::runtime_error(
					"There are some not deregistered cooperations (" +
					std::to_string( ids.size() ) + ")" );
		}
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

