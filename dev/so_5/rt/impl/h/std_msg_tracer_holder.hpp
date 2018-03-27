/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Standard implementation of message tracer holder.
 *
 * \since
 * v.5.5.22
 */

#pragma once

#include <so_5/h/msg_tracing.hpp>
#include <so_5/h/spinlocks.hpp>

#include <mutex>

namespace so_5 {

namespace msg_tracing {

namespace impl {

//
// std_holder_t
//
/*!
 * \brief Standard implementation of message tracer holder.
 *
 * This class also contains additional feature: it allows to change
 * message tracing filter.
 *
 * \since
 * v.5.5.22
 */
class std_holder_t : public holder_t
	{
	public :
		//! Initializing constructor.
		std_holder_t(
			//! Optional message tracing filter.
			//! This can be empty pointer. It doesn't matter because
			//! a filter can be changed latter.
			filter_shptr_t filter,
			//! Message tracer.
			//! If it is an empty pointer then message tracing is disabled.
			//! This value can't be changed in the future.
			tracer_unique_ptr_t tracer )
			:	m_filter{ std::move(filter) }
			,	m_tracer{ std::move(tracer) }
			{}

		virtual bool
		is_msg_tracing_enabled() const SO_5_NOEXCEPT override
			{
				return nullptr != m_tracer.get();
			}

		virtual filter_shptr_t
		take_filter() SO_5_NOEXCEPT override
			{
				std::lock_guard< default_spinlock_t > l{ m_lock };

				return m_filter;
			}

		void
		change_filter( filter_shptr_t filter ) SO_5_NOEXCEPT
			{
				std::lock_guard< default_spinlock_t > l{ m_lock };

				m_filter = std::move(filter);
			}

		virtual tracer_t &
		tracer() const SO_5_NOEXCEPT override
			{
				return *m_tracer;
			}

	private :
		//! A lock for protecting filter object.
		default_spinlock_t m_lock;

		filter_shptr_t m_filter;

		const tracer_unique_ptr_t m_tracer;
	};

} /* namespace impl */

} /* namespace msg_tracing */

} /* namespace so_5 */

