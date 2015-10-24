/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Definition of the template class event_data.
*/


#if !defined( _SO_5__RT__EVENT_DATA_HPP_ )
#define _SO_5__RT__EVENT_DATA_HPP_

#include <so_5/rt/h/message.hpp>

#include <type_traits>

namespace so_5
{

namespace rt
{

//! Template for the message incapsulation.
/*!
	Used to wrap a message which is the source for the agent event.

	Usage sample:
	\code
	void
	a_sample_t::evt_smth(
		const so_5::rt::event_data_t< sample_message_t > & msg )
	{
		// ...
	}
	\endcode

	\tparam MSG type of the message. MSG must be derived from
	so_5::rt::message_t (or from so_5::rt::signal_t).
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

} /* namespace rt */

} /* namespace so_5 */

#endif

