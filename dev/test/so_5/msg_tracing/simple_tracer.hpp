#pragma once

#include <so_5/all.hpp>

#include <atomic>

using counter_t = std::atomic< unsigned int >;

class tracer_t : public so_5::msg_tracing::tracer_t
{
public :
	tracer_t(
		counter_t & counter,
		so_5::msg_tracing::tracer_unique_ptr_t actual_tracer )
		:	m_counter( counter )
		,	m_actual_tracer( std::move( actual_tracer ) )
	{}

	virtual void
	trace( const std::string & message ) SO_5_NOEXCEPT override
	{
		++m_counter;
		m_actual_tracer->trace( message );
	}

private :
	counter_t & m_counter;
	so_5::msg_tracing::tracer_unique_ptr_t m_actual_tracer;
};

