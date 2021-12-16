/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Various helpers to work with work thread factories.
 *
 * \since v.5.7.3
 */

#pragma once

#include <so_5/disp/abstract_work_thread.hpp>

namespace so_5 {

namespace disp {

namespace reuse {

/*!
 * \brief Mixin that holds optional work thread factory.
 *
 * Indended to be used as mixin for various disp_params_t classes.
 *
 * \since v.5.7.3
 */
template< typename Params >
class work_thread_factory_mixin_t
	{
		/*!
		 * Factory to be used.
		 *
		 * \note
		 * It can be nullptr.
		 */
		abstract_work_thread_factory_shptr_t m_factory;

	public :
		//! Getter for work thread factory.
		[[nodiscard]]
		abstract_work_thread_factory_shptr_t
		work_thread_factory() const noexcept
			{
				return m_factory;
			}

		friend void
		swap(
				work_thread_factory_mixin_t & a,
				work_thread_factory_mixin_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_factory, b.m_factory );
			}

		//! Setter for work thread factory.
		Params &
		work_thread_factory(
			abstract_work_thread_factory_shptr_t v ) noexcept
			{
				m_factory = std::move(v);
				return static_cast< Params & >(*this);
			}
	};

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

