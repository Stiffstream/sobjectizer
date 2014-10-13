/*
 * A sample for the usage of take_under_control method.
 */

#include <iostream>
#include <stdexcept>
#include <sstream>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Logger sample class.
// Object of that class will be used as user resource for cooperation.
class logger_t
{
	public :
		logger_t()
		{
			std::cout << "[log] -- logger created --" << std::endl;
		}
		~logger_t()
		{
			std::cout << "[log] -- logger destroyed --" << std::endl;
		}

		void
		log( const std::string & what )
		{
			std::cout << "[log] " << what << std::endl;
		}
};

// A signal for parent cooperation about child work finish.
struct msg_child_finished : public so_5::rt::signal_t {};

// A class of child agent.
class a_child_t
	:	public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public :
		a_child_t(
			so_5::rt::environment_t & env,
			std::string agent_name,
			const so_5::rt::mbox_t & parent_mbox,
			logger_t & logger )
			:	base_type_t( env )
			,	m_agent_name( std::move( agent_name ) )
			,	m_parent_mbox( parent_mbox )
			,	m_logger( logger )
		{
			m_logger.log( m_agent_name + ": created" );
		}
		~a_child_t()
		{
			m_logger.log( m_agent_name + ": destroyed" );
		}

		virtual void
		so_evt_start() override
		{
			m_logger.log( m_agent_name + ": finishing" );
			m_parent_mbox->deliver_signal< msg_child_finished >();
		}

	private :
		const std::string m_agent_name;
		const so_5::rt::mbox_t m_parent_mbox;
		logger_t & m_logger;
};

// A class of parent agent.
class a_parent_t
	:	public so_5::rt::agent_t
{
	typedef so_5::rt::agent_t base_type_t;

	public :
		a_parent_t(
			so_5::rt::environment_t & env,
			logger_t & logger,
			size_t child_count )
			:	base_type_t( env )
			,	m_logger( logger )
			,	m_child_count( child_count )
			,	m_child_finished( 0 )
		{
			m_logger.log( "parent created" );
		}

		~a_parent_t()
		{
			m_logger.log( "parent destroyed" );
		}

		virtual void
		so_define_agent() override
		{
			so_subscribe_self().event< msg_child_finished >(
					&a_parent_t::evt_child_finished );
		}

		void
		so_evt_start()
		{
			m_logger.log( "creating child cooperation..." );
			register_child_coop();
			m_logger.log( "child cooperation created" );
		}

		void
		evt_child_finished()
		{
			m_logger.log( "child_finished notification received" );

			++m_child_finished;
			if( m_child_finished >= m_child_count )
			{
				m_logger.log( "stopping so_environment..." );
				so_deregister_agent_coop_normally();
			}
		}

	private :
		logger_t & m_logger;

		const size_t m_child_count;
		size_t m_child_finished;

		void
		register_child_coop()
		{
			auto coop = so_environment().create_coop( "child" );
			coop->set_parent_coop_name( so_coop_name() );

			for( size_t i = 0; i != m_child_count; ++i )
			{
				std::ostringstream s;
				s << "a_child_" << i+1;

				coop->add_agent(
						new a_child_t(
								so_environment(),
								s.str(),
								so_direct_mbox(),
								m_logger ) );
			}

			so_environment().register_coop( std::move( coop ) );
		}
};

// The SObjectizer Environment initialization.
void
init( so_5::rt::environment_t & env )
{
	auto coop = env.create_coop( "parent" );
	auto logger = coop->take_under_control( new logger_t() );
	coop->add_agent( new a_parent_t( env, *logger, 2 ) );

	env.register_coop( std::move( coop ) );
}

int
main( int, char ** )
{
	try
	{
		so_5::launch( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

