/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.4
 *
 * \file
 * \brief Helpers for do rollback actions in the case of exception.
 */

#pragma once

namespace so_5 {

namespace details {

namespace rollback_on_exception_details {

/*!
 * \since
 * v.5.5.4
 *
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

template< typename Result, typename Main_Action, typename Rollback_Action >
struct executor
	{
		static Result
		exec(
			Main_Action main_action,
			rollbacker_t< Rollback_Action > & rollback )
			{
				auto r = main_action();
				rollback.commit();

				return r;
			}
	};

template< typename Main_Action, typename Rollback_Action >
struct executor< void, Main_Action, Rollback_Action >
	{
		static void
		exec( 
			Main_Action main_action,
			rollbacker_t< Rollback_Action > & rollback )
			{
				main_action();
				rollback.commit();
			}
	};

} /* namespace rollback_on_exception_details */

/*!
 * \since
 * v.5.5.4
 *
 * \brief Helper function for do some action with rollback in the case of
 * an exception.
 *
 * \tparam Main_Action type of lambda with main action.
 * \tparam Rollback_Action type of lambda with rollback action.
 */
template< typename Main_Action, typename Rollback_Action >
auto
do_with_rollback_on_exception(
	Main_Action main_action,
	Rollback_Action rollback_action )
	-> decltype(main_action())
	{
		using result_type = decltype(main_action());

		using namespace rollback_on_exception_details;

		rollbacker_t< Rollback_Action > rollbacker{ rollback_action };

		return executor< result_type, Main_Action, Rollback_Action >::exec(
				main_action, rollbacker );
	}

} /* namespace details */

} /* namespace so_5 */

