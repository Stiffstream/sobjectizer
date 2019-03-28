/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Various stuff related to worker thread's join.
 *
 * \since
 * v.5.6.0
 */

#pragma once

#include <so_5/exception.hpp>
#include <so_5/ret_code.hpp>
#include <so_5/current_thread_id.hpp>

namespace so_5 {

namespace impl {

//
// ensure_join_from_different_thread
//
/*!
 * \brief Ensures that join will be called from different thread.
 *
 * It's a mistake if thread::join() is called from the context of
 * thread to be joined. Unfortunately, different implementations
 * of stdlib work differently. There can be an exception. Or there
 * can be deadlock, or there can be memory damage.
 *
 * It is better to check for that error manually and throw an exception
 * if the error is detected.
 *
 * \since
 * v.5.6.0
 */
inline void
ensure_join_from_different_thread(
	current_thread_id_t thread_to_be_joined )
	{
		if( query_current_thread_id() == thread_to_be_joined )
			SO_5_THROW_EXCEPTION(
					rc_unable_to_join_thread_by_itself,
					"worker thread attempts to join() itself" );
	}

} /* namespace impl */

} /* namespace so_5 */

