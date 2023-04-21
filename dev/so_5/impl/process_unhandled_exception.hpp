/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.4.0
 *
 * \file
 * \brief Helpers for handling unhandled exceptions from agent's event handlers.
 */

#pragma once

#include <so_5/agent.hpp>

namespace so_5 {

namespace impl {

/*!
 * \since
 * v.5.4.0
 *
 * \brief Processor of unhandled exception from agent's event handler.
 *
 * \note
 * This function is noexcept since v.5.6.2.
 */
void
process_unhandled_exception(
	//! ID of working thread for the problematic agent.
	//! NULL thread id means that agent is multithreadded.
	//! For such cases only subset of exception_reaction is
	//! allowed (ignore_exception, abort_on_exception).
	current_thread_id_t working_thread_id,
	//! Raised and caught exception.
	const std::exception & ex,
	//! Agent who is the producer of the exception.
	agent_t & a_exception_producer ) noexcept;

/*!
 * \since
 * v.5.5.24.3
 *
 * \brief Processor of unhandled exception of unknown type from agent's event
 * handler.
 *
 * \note
 * This function will be called from catch(...) and because of that there
 * is no actual exception object.
 */
void
process_unhandled_unknown_exception(
	//! ID of working thread for the problematic agent.
	//! NULL thread id means that agent is multithreadded.
	//! For such cases only subset of exception_reaction is
	//! allowed (ignore_exception, abort_on_exception).
	current_thread_id_t working_thread_id,
	//! Agent who is the producer of the exception.
	agent_t & a_exception_producer ) noexcept;

} /* namespace impl */

} /* namespace so_5 */

