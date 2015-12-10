/*
 * A test for coop reg/dereg notifications order.
 */

#include <iostream>
#include <sstream>
#include <mutex>

#include <so_5/all.hpp>

struct msg_child_deregistered : public so_5::signal_t {};

class a_child_t : public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public :
		a_child_t(
			so_5::environment_t & env )
			:	base_type_t( env )
		{
		}

		void
		so_evt_start()
		{
			so_environment().deregister_coop(
					so_coop_name(),
					so_5::dereg_reason::normal );
		}
};

class a_test_t : public so_5::agent_t
{
	typedef so_5::agent_t base_type_t;

	public :
		a_test_t(
			so_5::environment_t & env,
			const so_5::coop_reg_notificator_t & reg_notificator,
			const so_5::coop_dereg_notificator_t & dereg_notificator )
			:	base_type_t( env )
			,	m_reg_notificator( reg_notificator )
			,	m_dereg_notificator( dereg_notificator )
			,	m_mbox( env.create_mbox() )
		{}

		void
		so_define_agent()
		{
			so_subscribe( m_mbox ).event( &a_test_t::evt_child_deregistered );
		}

		void
		so_evt_start()
		{
			auto child_coop = so_environment().create_coop(
					"child",
					so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

			child_coop->set_parent_coop_name( so_coop_name() );
			child_coop->add_reg_notificator( m_reg_notificator );
			child_coop->add_dereg_notificator( m_dereg_notificator );
			child_coop->add_dereg_notificator(
					[this]( so_5::environment_t &,
						const std::string &,
						const so_5::coop_dereg_reason_t &)
					{
						m_mbox->deliver_signal< msg_child_deregistered >();
					} );

			child_coop->add_agent( new a_child_t( so_environment() ) );

			so_environment().register_coop( std::move( child_coop ) );
		}

		void
		evt_child_deregistered(
			const so_5::event_data_t< msg_child_deregistered > & )
		{
			so_environment().stop();
		}

	private :
		const so_5::coop_reg_notificator_t m_reg_notificator;
		const so_5::coop_dereg_notificator_t m_dereg_notificator;

		const so_5::mbox_t m_mbox;
};

class sequence_holder_t
{
	public :
		typedef std::vector< std::string > seq_t;

		void
		add( const std::string & msg )
		{
			std::lock_guard< std::mutex > lock( m_lock );
			m_seq.push_back( msg );
		}

		const seq_t
		sequence() const
		{
			return m_seq;
		}

	private :
		std::mutex m_lock;

		seq_t m_seq;
};

std::string
sequence_to_string(
	const sequence_holder_t::seq_t & what )
{
	std::string result;
	for( auto i = what.begin(); i != what.end(); ++i )
	{
		if( i != what.begin() )
			result += ",";
		result += *i;
	}

	return result;
}

class test_env_t
{
	public :
		test_env_t()
		{}

		void
		init( so_5::environment_t & env )
		{
			env.add_dispatcher_if_not_exists(
					"active_obj",
					[] { return so_5::disp::active_obj::create_disp(); } );

			env.register_agent_as_coop(
					"test",
					new a_test_t(
							env,
							create_on_reg_notificator(),
							create_on_dereg_notificator() ) );
		}

		void
		check_result() const
		{
			sequence_holder_t::seq_t expected;
			expected.push_back( "on_reg_1" );
			expected.push_back( "on_reg_2" );
			expected.push_back( "on_dereg" );

			if( m_sequence.sequence() != expected )
				throw std::runtime_error( "Wrong notification sequence! "
						"actual: '" + sequence_to_string( m_sequence.sequence() ) +
						"', expected: '" + sequence_to_string( expected ) );
		}

	private :
		sequence_holder_t m_sequence;

		so_5::coop_reg_notificator_t
		create_on_reg_notificator()
		{
			return [this]( so_5::environment_t &,
							const std::string & )
					{
						m_sequence.add( "on_reg_1" );

						// A timeout for child coop deregistration initiation.
						std::this_thread::sleep_for(
								std::chrono::seconds(1) );

						m_sequence.add( "on_reg_2" );
					};
		}

		so_5::coop_dereg_notificator_t
		create_on_dereg_notificator()
		{
			return [this]( so_5::environment_t &,
							const std::string &,
							const so_5::coop_dereg_reason_t &)
					{
						m_sequence.add( "on_dereg" );
					};
		}
};

int
main()
{
	try
	{
		test_env_t test_env;
		so_5::launch(
			[&test_env]( so_5::environment_t & env )
			{
				test_env.init( env );
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

