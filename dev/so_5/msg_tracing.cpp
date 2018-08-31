/*
 * SObjectizer-5
 */

#include <so_5/h/msg_tracing.hpp>

#include <mutex>
#include <iostream>

namespace so_5 {

namespace msg_tracing {

namespace impl {

//
// std_stream_tracer_t
//
/*!
 * \since
 * v.5.5.9
 *
 * \brief A simple implementation of tracer which uses one of
 * standard streams.
 */
class std_stream_tracer_t : public tracer_t
	{
	public :
		//! Main constructor.
		std_stream_tracer_t(
			//! Stream to be used for tracing.
			std::ostream & stream )
			:	m_stream( stream )
			{}

		virtual void
		trace( const std::string & what ) SO_5_NOEXCEPT override
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				m_stream << what << std::endl;
			}

	private :
		//! Object lock.
		/*!
		 * It is necessary for synchronization of trace calls from
		 * different threads.
		 */
		std::mutex m_lock;

		//! Stream to be used for tracing.
		std::ostream & m_stream;
	};

} /* namespace impl */

//
// Standard stream tracers.
//

SO_5_FUNC tracer_unique_ptr_t
std_cout_tracer()
	{
		return tracer_unique_ptr_t{ new impl::std_stream_tracer_t{ std::cout } };
	}

SO_5_FUNC tracer_unique_ptr_t
std_cerr_tracer()
	{
		return tracer_unique_ptr_t{ new impl::std_stream_tracer_t{ std::cerr } };
	}

SO_5_FUNC tracer_unique_ptr_t
std_clog_tracer()
	{
		return tracer_unique_ptr_t{ new impl::std_stream_tracer_t{ std::clog } };
	}

} /* namespace msg_tracing */

} /* namespace so_5 */


