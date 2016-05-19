/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.4
 * \file
 * \brief Helpers for do rollback actions in the case of exception.
 */

#pragma once

namespace so_5 {

namespace details {

namespace rollback_on_exception_details {

/*!
 * \since v.5.5.4
 * \brief Helper template class for do rollback actions automatically
 * in the destructor.
 *
 * \tparam L type of lambda with rollback actions.
 */
template< typename L >
class rollbacker_t
	{
		L & m_action;
		bool m_commited = false;

	public :
		inline rollbacker_t( L & action ) : m_action( action ) {}
		inline ~rollbacker_t() { if( !m_commited ) m_action(); }

		inline void commit() { m_commited = true; }
	};

template< typename RESULT, typename MAIN_ACTION, typename ROLLBACK_ACTION >
struct executor
	{
		static RESULT
		exec(
			MAIN_ACTION main_action,
			rollbacker_t< ROLLBACK_ACTION > & rollback )
			{
				auto r = main_action();
				rollback.commit();

				return r;
			}
	};

template< typename MAIN_ACTION, typename ROLLBACK_ACTION >
struct executor< void, MAIN_ACTION, ROLLBACK_ACTION >
	{
		static void
		exec( 
			MAIN_ACTION main_action,
			rollbacker_t< ROLLBACK_ACTION > & rollback )
			{
				main_action();
				rollback.commit();
			}
	};

} /* namespace rollback_on_exception_details */

/*!
 * \since v.5.5.4
 * \brief Helper function for do some action with rollback in the case of
 * an exception.
 *
 * \tparam MAIN_ACTION type of lambda with main action.
 * \tparam ROLLBACK_ACTION type of lambda with rollback action.
 */
template< typename MAIN_ACTION, typename ROLLBACK_ACTION >
auto
do_with_rollback_on_exception(
	MAIN_ACTION main_action,
	ROLLBACK_ACTION rollback_action )
	-> decltype(main_action())
	{
		using result_type = decltype(main_action());

		using namespace rollback_on_exception_details;

		rollbacker_t< ROLLBACK_ACTION > rollbacker{ rollback_action };

		return executor< result_type, MAIN_ACTION, ROLLBACK_ACTION >::exec(
				main_action, rollbacker );
	}

} /* namespace details */

} /* namespace so_5 */

