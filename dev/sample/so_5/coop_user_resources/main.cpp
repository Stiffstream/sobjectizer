/*
 * A sample for the usage of take_under_control method.
 */

#include <iostream>
#include <stdexcept>
#include <string>

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
struct msg_child_finished : public so_5::signal_t {};

// A class of child agent.
class a_child_t :	public so_5::agent_t
{
	public :
		a_child_t(
			context_t ctx,
			std::string agent_name,
			so_5::mbox_t parent_mbox,
			logger_t & logger )
			:	so_5::agent_t( ctx )
			,	m_agent_name( std::move( agent_name ) )
			,	m_parent_mbox( std::move( parent_mbox ) )
			,	m_logger( logger )
		{
			m_logger.log( m_agent_name + ": created" );
		}
		~a_child_t() override
		{
			m_logger.log( m_agent_name + ": destroyed" );
		}

		virtual void so_evt_start() override
		{
			m_logger.log( m_agent_name + ": finishing" );
			so_5::send< msg_child_finished >( m_parent_mbox );
		}

	private :
		const std::string m_agent_name;
		const so_5::mbox_t m_parent_mbox;
		logger_t & m_logger;
};

// A class of parent agent.
class a_parent_t : public so_5::agent_t
{
	public :
		a_parent_t(
			context_t ctx,
			logger_t & logger,
			size_t child_count )
			:	so_5::agent_t( ctx )
			,	m_logger( logger )
			,	m_child_count( child_count )
			,	m_child_finished( 0 )
		{
			m_logger.log( "parent created" );
		}

		~a_parent_t() override
		{
			m_logger.log( "parent destroyed" );
		}

		virtual void so_define_agent() override
		{
			so_default_state().event< msg_child_finished >(
					&a_parent_t::evt_child_finished );
		}

		void so_evt_start() override
		{
			m_logger.log( "creating child cooperation..." );
			register_child_coop();
			m_logger.log( "child cooperation created" );
		}

		void evt_child_finished()
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

		void register_child_coop()
		{
			so_5::introduce_child_coop( *this, "child",
				[this]( so_5::coop_t & coop ) 
				{
					for( size_t i = 0; i != m_child_count; ++i )
						coop.make_agent< a_child_t >(
								"a_child_" + std::to_string(i+1),
								so_direct_mbox(), m_logger );
				} );
		}
};

// The SObjectizer Environment initialization.
void init( so_5::environment_t & env )
{
	env.introduce_coop( []( so_5::coop_t & coop ) {
		auto logger = coop.take_under_control( new logger_t() );
		coop.make_agent< a_parent_t >( *logger, 2u );
	} );
}

int main()
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

