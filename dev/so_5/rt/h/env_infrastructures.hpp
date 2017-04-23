/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief User-visible parameters for various environment infrastructures.
 *
 * \since
 * v.5.5.19
 */

#pragma once

#include <so_5/h/timers.hpp>

#include <so_5/rt/h/environment_infrastructure.hpp>

namespace so_5 {

namespace env_infrastructures {

namespace default_mt {

// NOTE: implemented in so_5/rt/impl/mt_env_infrastructure.cpp
/*!
 * \brief A factory for creation the default multitheading
 * environment infrastructure.
 *
 * Usage example:
 * \code
   so_5::launch(
			[](so_5::environment_t & env) { ... },
			[](so_5::environment_params_t & params) {
				params.infrastructure_factory(
						so_5::env_infrastructures::default_mt::factory() );
				...
			} );
 * \endcode
 *
 * \note
 * This factory is used by default.
 *
 * \since
 * v.5.5.19
 */
SO_5_FUNC environment_infrastructure_factory_t
factory();

} /* namespace default_mt */

namespace simple_mtsafe {

//
// params_t
//
/*!
 * \brief Parameters for simple thread-safe single-thread environment.
 *
 * Usage example:
 * \code
   so_5::env_infrastructures::simple_mtsafe::params_t params;
	params.timer_manager( so_5::timer_list_manager_factory() );

	env_params.infrastructure_factory( factory(std::move(params)) );
 * \endcode
 *
 * \since
 * v.5.5.19
 */
class params_t
	{
		//! Timer manager factory for environment.
		/*!
		 * thread_heap mechanism is used by default.
		 */
		timer_manager_factory_t m_timer_factory{ timer_heap_manager_factory() };

	public :
		//! Setter for timer_manager factory.
		params_t &
		timer_manager( timer_manager_factory_t factory ) SO_5_OVERLOAD_FOR_REF
			{
				m_timer_factory = std::move(factory);
				return *this;
			}

#if !defined( SO_5_NO_SUPPORT_FOR_RVALUE_REFERENCE_OVERLOADING )
		//! Setter for timer_manager factory.
		params_t &&
		timer_manager( timer_manager_factory_t factory ) SO_5_OVERLOAD_FOR_RVALUE_REF
			{
				m_timer_factory = std::move(factory);
				return std::move(*this);
			}
#endif

		//! Getter for timer_manager factory.
		const timer_manager_factory_t &
		timer_manager() const
			{
				return m_timer_factory;
			}
	};

// NOTE: implemented in so_5/rt/impl/simple_mtsafe_st_env_infastructure.cpp
//
// factory
//
/*!
 * \brief A factory for creation of simple thread-safe
 * single-thread environment infrastructure object.
 *
 * Usage example:
 * \code
   so_5::launch(
			[](so_5::environment_t & env) { ... },
			[](so_5::environment_params_t & params) {
				params.infrastructure_factory(
						factory( so_5::env_infrastructures::simple_mtsafe::params_t{}
								.timer_manager( so_5::timer_list_manager_factory() ) ) );
				...
			} );
 * \endcode
 *
 * \since
 * v.5.5.19
 */
SO_5_FUNC environment_infrastructure_factory_t
factory( params_t && params );

/*!
 * \brief A factory for creation of simple thread-safe
 * single-thread environment infrastructure object with
 * default parameters.
 *
 * Usage example:
 * \code
   so_5::launch(
			[](so_5::environment_t & env) { ... },
			[](so_5::environment_params_t & params) {
				params.infrastructure_factory(
						so_5::env_infrastructures::simple_mtsafe::factory() );
				...
			} );
 * \endcode
 *
 * \since
 * v.5.5.19
 */
inline environment_infrastructure_factory_t
factory()
	{
		return factory( params_t() );
	}

} /* namespace simple_mtsafe */

namespace simple_not_mtsafe {

//
// params_t
//
/*!
 * \brief Parameters for simple not-thread-safe single-thread environment.
 *
 * Usage example:
 * \code
   so_5::env_infrastructures::simple_not_mtsafe::params_t params;
	params.timer_manager( so_5::timer_list_manager_factory() );

	env_params.infrastructure_factory( factory(std::move(params)) );
 * \endcode
 *
 * \since
 * v.5.5.19
 */
class params_t
	{
		//! Timer manager factory for environment.
		/*!
		 * thread_heap mechanism is used by default.
		 */
		timer_manager_factory_t m_timer_factory{ timer_heap_manager_factory() };

	public :
		//! Setter for timer_manager factory.
		params_t &
		timer_manager( timer_manager_factory_t factory ) SO_5_OVERLOAD_FOR_REF
			{
				m_timer_factory = std::move(factory);
				return *this;
			}

#if !defined( SO_5_NO_SUPPORT_FOR_RVALUE_REFERENCE_OVERLOADING )
		//! Setter for timer_manager factory.
		params_t &&
		timer_manager( timer_manager_factory_t factory ) SO_5_OVERLOAD_FOR_RVALUE_REF
			{
				m_timer_factory = std::move(factory);
				return std::move(*this);
			}
#endif

		//! Getter for timer_manager factory.
		const timer_manager_factory_t &
		timer_manager() const
			{
				return m_timer_factory;
			}
	};

// NOTE: implemented in so_5/rt/impl/simple_not_mtsafe_st_env_infastructure.cpp
//
// factory
//
/*!
 * \brief A factory for creation of simple not-thread-safe
 * single-thread environment infrastructure object.
 *
 * Usage example:
 * \code
   so_5::launch(
			[](so_5::environment_t & env) { ... },
			[](so_5::environment_params_t & params) {
				params.infrastructure_factory(
						factory( so_5::env_infrastructures::simple_not_mtsafe::params_t{}
								.timer_manager( so_5::timer_list_manager_factory() ) ) );
				...
			} );
 * \endcode
 *
 * \since
 * v.5.5.19
 */
SO_5_FUNC environment_infrastructure_factory_t
factory( params_t && params );

/*!
 * \brief A factory for creation of simple not-thread-safe
 * single-thread environment infrastructure object with
 * default parameters.
 *
 * Usage example:
 * \code
   so_5::launch(
			[](so_5::environment_t & env) { ... },
			[](so_5::environment_params_t & params) {
				params.infrastructure_factory(
						so_5::env_infrastructures::simple_not_mtsafe::factory() );
				...
			} );
 * \endcode
 *
 * \since
 * v.5.5.19
 */
inline environment_infrastructure_factory_t
factory()
	{
		return factory( params_t() );
	}

} /* namespace simple_not_mtsafe */

} /* namespace env_infrastructures */

} /* namespace so_5 */

