/*
	Watchdog agent public Iface.
*/

#pragma once

#include <chrono>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Forward declarations for used classes.
class watchdog_t;
class a_watchdog_t;
class a_watchdog_impl_t;


// The RAII idiom operation watchdog helper.
class operation_watchdog_t
{
		operation_watchdog_t( const operation_watchdog_t & );
		void
		operator = ( const operation_watchdog_t & );

	public:
		operation_watchdog_t(
			const watchdog_t & watchdog,
			const std::string & operation_tag,
			const std::chrono::steady_clock::duration & timeout );

		~operation_watchdog_t();

	private:
		const watchdog_t & m_watchdog;
		const std::string m_operation_tag;
};

// Watchdog bind.
// Is incleded to watched agents and
// gives interface for controlling running operations.
class watchdog_t
{
		friend class a_watchdog_t;
		explicit watchdog_t( const so_5::rt::mbox_t & mbox );

	public:
		~watchdog_t();

		void
		start_watch_operation(
			const std::string & operation_tag,
			const std::chrono::steady_clock::duration & timeout ) const;

		void
		stop_watch_operation( const std::string & tag ) const;

	private:
		const so_5::rt::mbox_t m_mbox;
};

// Watchdog agent.
class  a_watchdog_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:
		a_watchdog_t(
			so_5::rt::environment_t & env,
			const std::chrono::steady_clock::duration & check_interval =
				std::chrono::seconds( 1 ) );

		virtual ~a_watchdog_t();

		virtual void
		so_define_agent() override;

		virtual void
		so_evt_start() override;

		// Create watchdog associated with the agent.
		watchdog_t
		create_watchdog() const;

	private:
		std::unique_ptr< a_watchdog_impl_t > m_watchdog_impl;
};
