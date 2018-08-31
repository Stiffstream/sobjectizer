/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.4
 *
 * \brief A standard implementation of controller for run-time monitoring.
 */

#include <so_5/rt/stats/impl/h/std_controller.hpp>

#include <so_5/rt/stats/h/messages.hpp>

#include <so_5/rt/h/send_functions.hpp>

namespace so_5
{

namespace stats
{

namespace impl
{

//
// std_controller_t
//

std_controller_t::std_controller_t(
	mbox_t mbox )
	:	m_mbox( std::move( mbox ) )
	{}

const mbox_t &
std_controller_t::mbox() const
	{
		return m_mbox;
	}

void
std_controller_t::turn_on()
	{
		std::lock_guard< std::mutex > lock{ m_start_stop_lock };

		if( !m_distribution_thread )
			{
				// Distribution thread must be started.
				m_shutdown_initiated = false;
				m_distribution_thread.reset(
						new std::thread( [this] { body(); } ) );
			}
	}

void
std_controller_t::turn_off()
	{
		std::lock_guard< std::mutex > lock{ m_start_stop_lock };

		if( m_distribution_thread )
			{
				{
					// Send shutdown signal to work thread.
					std::lock_guard< std::mutex > cond_lock{ m_data_lock };
					m_shutdown_initiated = true;

					m_wake_up_cond.notify_one();
				}

				// Wait for work thread termination.
				m_distribution_thread->join();

				// Pointer to work thread must be dropped.
				// This allows to start new working thread.
				m_distribution_thread.reset();
			}
	}

std::chrono::steady_clock::duration
std_controller_t::set_distribution_period(
	std::chrono::steady_clock::duration period )
	{
		std::lock_guard< std::mutex > lock{ m_data_lock };
		
		auto ret_value = m_distribution_period;

		m_distribution_period = period;

		return ret_value;
	}

void
std_controller_t::add( source_t & what )
	{
		std::lock_guard< std::mutex > lock{ m_data_lock };

		source_list_add( what, m_head, m_tail );
	}

void
std_controller_t::remove( source_t & what )
	{
		std::lock_guard< std::mutex > lock{ m_data_lock };

		source_list_remove( what, m_head, m_tail );
	}

void
std_controller_t::body()
	{
		while( true )
			{
				std::unique_lock< std::mutex > lock{ m_data_lock };

				if( m_shutdown_initiated )
					return;

				const auto actual_duration = distribute_current_data();

				if( actual_duration < m_distribution_period )
					// There is some time to sleep.
					m_wake_up_cond.wait_for(
							lock,
							m_distribution_period - actual_duration );
			}
	}

std::chrono::steady_clock::duration
std_controller_t::distribute_current_data()
	{
		auto started_at = std::chrono::steady_clock::now();

		send< so_5::stats::messages::distribution_started >( m_mbox );

		source_t * s = m_head;
		while( s )
			{
				s->distribute( m_mbox );

				s = source_list_next( *s );
			}

		send< so_5::stats::messages::distribution_finished >( m_mbox );

		return std::chrono::steady_clock::now() - started_at;
	}

} /* namespace impl */

} /* namespace stats */

} /* namespace so_5 */

