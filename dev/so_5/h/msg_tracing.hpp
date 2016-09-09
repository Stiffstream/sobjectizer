/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.9
 *
 * \file
 * \brief Stuff related to message delivery tracing.
 */

#pragma once

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>

#include <string>
#include <memory>

namespace so_5 {

namespace msg_tracing {

/*!
 * \since
 * v.5.5.9
 *
 * \brief Status of message delivery tracing.
 */
enum class status_t
	{
		//! Message delivery tracing is disabled.
		disabled,
		//! Message delivery tracing is enabled.
		enabled
	};

//
// tracer_t
//

/*!
 * \since
 * v.5.5.9
 *
 * \brief Interface of tracer object.
 */
class SO_5_TYPE tracer_t
	{
		// Note: clang-3.9 requires this on Windows platform.
		tracer_t( const tracer_t & ) = delete;
		tracer_t( tracer_t && ) = delete;
		tracer_t & operator=( const tracer_t & ) = delete;
		tracer_t & operator=( tracer_t && ) = delete;
	public :
		tracer_t();
		virtual ~tracer_t();

		//! Store a description of message delivery action to the
		//! appropriate storage/stream.
		virtual void
		trace( const std::string & what ) SO_5_NOEXCEPT = 0;
	};

//
// tracer_unique_ptr_t
//

/*!
 * \since
 * v.5.5.9
 *
 * \brief A short alias for unique_ptr to tracer.
 */
using tracer_unique_ptr_t = std::unique_ptr< tracer_t >;

//
// Standard stream tracers.
//

/*!
 * \since
 * v.5.5.9
 *
 * \brief Factory for tracer which uses std::cout stream.
 */
SO_5_FUNC tracer_unique_ptr_t
std_cout_tracer();

/*!
 * \since
 * v.5.5.9
 *
 * \brief Factory for tracer which uses std::cerr stream.
 */
SO_5_FUNC tracer_unique_ptr_t
std_cerr_tracer();

/*!
 * \since
 * v.5.5.9
 *
 * \brief Factory for tracer which uses std::clog stream.
 */
SO_5_FUNC tracer_unique_ptr_t
std_clog_tracer();

} /* namespace msg_tracing */

} /* namespace so_5 */

