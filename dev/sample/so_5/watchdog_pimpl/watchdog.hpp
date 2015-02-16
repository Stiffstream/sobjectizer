/*
	Watchdog agent public Iface.
*/

#pragma once

#include <chrono>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Forward declarations for used classes.
class a_watchdog_t;
class a_watchdog_impl_t;

// The RAII idiom operation watchdog helper.
class operation_watchdog_t
{
		operation_watchdog_t( const operation_watchdog_t & ) = delete;
		void operator=( const operation_watchdog_t & ) = delete;

	public:
		operation_watchdog_t(
			a_watchdog_t * watchdog_agent,
			std::string tag,
			const std::chrono::steady_clock::duration & timeout );

		~operation_watchdog_t();

	private:
		a_watchdog_t * m_watchdog_agent;
		const std::string m_tag;
};

// Watchdog agent.
class  a_watchdog_t : public so_5::rt::agent_t
{
	public:
		a_watchdog_t( so_5::rt::environment_t & env );
		virtual ~a_watchdog_t();

		virtual void
		so_define_agent() override;

	private:
		std::unique_ptr< a_watchdog_impl_t > m_impl;
};

