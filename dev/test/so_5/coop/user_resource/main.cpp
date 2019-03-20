/*
 * A test for deallocating user resources.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct sequence_holder_t
{
	typedef std::vector< int > vec_t;

	vec_t m_sequence;
};

const int ID_COOP = 1;
const int ID_RESOURCE = 2;
const int ID_AGENT = 3;

struct resource_t
{
	sequence_holder_t & m_holder;

	resource_t( sequence_holder_t & holder )
		:	m_holder( holder )
	{}
	~resource_t()
	{
		m_holder.m_sequence.push_back( ID_RESOURCE );
	}
};

class a_test_t : public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public :
		a_test_t(
			so_5::environment_t & env,
			resource_t & resource )
			:	base_type_t( env )
			,	m_resource( resource )
		{
		}
		~a_test_t()
		{
			m_resource.m_holder.m_sequence.push_back( ID_AGENT );
		}

		void
		so_evt_start()
		{
			so_deregister_agent_coop_normally();
		}

	private :
		resource_t & m_resource;
};

class test_agent_coop_t
	:	public so_5::coop_t
{
	typedef so_5::coop_t base_type_t;
	public :
		test_agent_coop_t(
			so_5::coop_id_t id,
			so_5::coop_handle_t parent_coop,
			so_5::disp_binder_shptr_t coop_disp_binder,
			so_5::environment_t & env,
			sequence_holder_t & sequence )
			:	base_type_t{
					id,
					std::move(parent_coop),
					std::move(coop_disp_binder),
					so_5::outliving_mutable(env) }
			,	m_sequence( sequence )
		{}

		~test_agent_coop_t()
		{
			m_sequence.m_sequence.push_back( ID_COOP );
		}

	private :
		sequence_holder_t & m_sequence;
};

class a_test_starter_t : public so_5::agent_t
{
	typedef so_5::agent_t base_type_t;

	public :
		a_test_starter_t(
			so_5::environment_t & env,
			sequence_holder_t & sequence )
			:	base_type_t( env )
			,	m_sequence( sequence )
			,	m_self_mbox( env.create_mbox() )
		{}

		void
		so_define_agent()
		{
			so_subscribe( m_self_mbox ).event(
					&a_test_starter_t::evt_child_destroyed );
		}

		void
		so_evt_start()
		{
			so_5::coop_unique_holder_t coop(
					so_5::coop_shptr_t{
							new test_agent_coop_t(
									1234567u,
									so_coop(),
									so_5::make_default_disp_binder( so_environment() ),
									so_environment(),
									m_sequence )
					} );
			coop->add_dereg_notificator(
					so_5::make_coop_dereg_notificator( m_self_mbox ) );

			resource_t * r = coop->take_under_control(
					std::make_unique< resource_t >( m_sequence ) );
			coop->make_agent< a_test_t >( *r );

			so_environment().register_coop( std::move( coop ) );
		}

		void
		evt_child_destroyed(
			mhood_t< so_5::msg_coop_deregistered > )
		{
			so_environment().stop();
		}

	private :
		sequence_holder_t & m_sequence;

		const so_5::mbox_t m_self_mbox;
};

std::string
sequence_to_string( const std::vector< int > & s )
{
	std::ostringstream out;
	for( auto i = s.begin(); i != s.end(); ++i )
	{
		if( i != s.begin() )
			out << ", ";
		out << *i;
	}

	return out.str();
}

class test_env_t
{
	public :
		void
		init( so_5::environment_t & env )
		{
			env.register_agent_as_coop(
					env.make_agent< a_test_starter_t >( m_sequence ) );
		}

		void
		check_result() const
		{
			sequence_holder_t::vec_t expected1{ ID_COOP, ID_AGENT, ID_RESOURCE };
			sequence_holder_t::vec_t expected2{ ID_AGENT, ID_RESOURCE, ID_COOP };

			if( m_sequence.m_sequence != expected1 &&
					m_sequence.m_sequence != expected2 )
				throw std::runtime_error( "Wrong deinit sequence:\n"
						"actual: " + sequence_to_string( m_sequence.m_sequence )
						+ "\n" "expected1: " + sequence_to_string( expected1 ) +
						"\n" "expected2: " + sequence_to_string( expected2 ) );
		}

	private :
		sequence_holder_t m_sequence;
};

int
main()
{
	run_with_time_limit( [] {
			test_env_t test_env;
			so_5::launch(
				[&]( so_5::environment_t & env ) {
					test_env.init( env );
				} );

			test_env.check_result();
		},
		10 );

	return 0;
}

