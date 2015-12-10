/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Definition of the template class event_data.
*/

#pragma once

#include <so_5/rt/h/message.hpp>

#include <type_traits>

namespace so_5
{

//! Template for the message incapsulation.
/*!
	Used to wrap a message which is the source for the agent event.

	Usage sample:
	\code
	void
	a_sample_t::evt_smth(
		const so_5::event_data_t< sample_message_t > & msg )
	{
		// ...
	}
	\endcode

	\tparam MSG type of the message. MSG must be derived from
	so_5::message_t (or from so_5::signal_t).
*/
template< class MSG >
class event_data_t
{
	public:
		//! Constructor.
		event_data_t( MSG * message_instance )
			:	m_message_instance( message_instance )
		{
			ensure_classical_message< MSG >();
		}

		//! Access to the message.
		const MSG&
		operator * () const
		{
			ensure_not_signal< MSG >();

			return *m_message_instance;
		}

		//! Access to the raw message pointer.
		const MSG *
		get() const
		{
			ensure_not_signal< MSG >();

			return m_message_instance;
		}

		//! Access to the message via pointer.
		const MSG *
		operator -> () const
		{
			ensure_not_signal< MSG >();

			return get();
		}

		intrusive_ptr_t< MSG >
		make_reference() const
		{
			return intrusive_ptr_t< MSG >( m_message_instance );
		}

	private:
		//! Message.
		MSG * const m_message_instance;
};

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::event_data_t instead.
 */
template< class MSG >
using event_data_t = so_5::event_data_t< MSG >;

} /* namespace rt */

} /* namespace so_5 */

