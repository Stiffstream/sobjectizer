/*
 * A test for deallocating user resources.
 */

#include <iostream>
#include <sstream>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

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

class a_test_t : public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public :
		a_test_t(
			so_5::rt::so_environment_t & env,
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
			so_environment().deregister_coop(
					so_coop_name(),
					so_5::rt::dereg_reason::normal );
		}

	private :
		resource_t & m_resource;
};

class test_agent_coop_t
	:	public so_5::rt::agent_coop_t
{
	typedef so_5::rt::agent_coop_t base_type_t;
	public :
		test_agent_coop_t(
			const so_5::rt::nonempty_name_t & name,
			so_5::rt::disp_binder_unique_ptr_t coop_disp_binder,
			so_5::rt::so_environment_t & env,
			sequence_holder_t & sequence )
			:	base_type_t( name, std::move(coop_disp_binder), env )
			,	m_sequence( sequence )
		{}

	protected :
		~test_agent_coop_t()
		{
			m_sequence.m_sequence.push_back( ID_COOP );
		}

	private :
		sequence_holder_t & m_sequence;
};

class a_test_starter_t : public so_5::rt::agent_t
{
	typedef so_5::rt::agent_t base_type_t;

	public :
		a_test_starter_t(
			so_5::rt::so_environment_t & env,
			sequence_holder_t & sequence )
			:	base_type_t( env )
			,	m_sequence( sequence )
			,	m_self_mbox( env.create_local_mbox() )
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
			so_5::rt::agent_coop_unique_ptr_t coop(
					new test_agent_coop_t(
							"child_coop",
							so_5::rt::create_default_disp_binder(),
							so_environment(),
							m_sequence ) );
			coop->set_parent_coop_name( so_coop_name() );
			coop->add_dereg_notificator(
					so_5::rt::make_coop_dereg_notificator( m_self_mbox ) );

			resource_t * r = coop->take_under_control(
					new resource_t( m_sequence ) );
			coop->add_agent( new a_test_t( so_environment(), *r ) );

			so_environment().register_coop( std::move( coop ) );
		}

		void
		evt_child_destroyed(
			const so_5::rt::event_data_t<
				so_5::rt::msg_coop_deregistered > & evt )
		{
			so_environment().stop();
		}

	private :
		sequence_holder_t & m_sequence;

		const so_5::rt::mbox_ref_t m_self_mbox;
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
		init( so_5::rt::so_environment_t & env )
		{
			env.register_agent_as_coop(
					"starter", new a_test_starter_t( env, m_sequence ) );
		}

		void
		check_result() const
		{
			sequence_holder_t::vec_t expected;
			expected.push_back( ID_COOP );
			expected.push_back( ID_AGENT );
			expected.push_back( ID_RESOURCE );

			if( m_sequence.m_sequence != expected )
				throw std::runtime_error( "Wrong deinit sequence: expected: " +
						sequence_to_string( expected ) +
						", actual: " +
						sequence_to_string( m_sequence.m_sequence ) );
		}

	private :
		sequence_holder_t m_sequence;
};

int
main( int argc, char * argv[] )
{
	try
	{
		test_env_t test_env;
		so_5::api::run_so_environment_on_object(
				test_env,
				&test_env_t::init,
				so_5::rt::so_environment_params_t() );

		test_env.check_result();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

