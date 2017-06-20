/*
 * SObjectizer 5
 */

/*!
 * \file
 * \brief Various stuff required for custom mboxes.
 *
 * \since
 * v.5.5.19.2
 */

#pragma once

#include <so_5/rt/h/mbox.hpp>

#include <so_5/h/msg_tracing.hpp>

namespace so_5 {

//
// mbox_creation_data_t
//
/*!
 * \brief An information which is necessary for creation of a new mbox.
 *
 * \since
 * v.5.5.19.2
 */
struct mbox_creation_data_t
	{
		//! ID for a new mbox.
		mbox_id_t m_id;
		//! Tracer object to be used for message tracing.
		/*!
		 * Can be nullptr. In that case message tracing must not be used.
		 */
		msg_tracing::tracer_t * m_tracer;

		//! Initializing constructor.
		mbox_creation_data_t(
			mbox_id_t id,
			msg_tracing::tracer_t * tracer )
			:	m_id(id)
			,	m_tracer(tracer)
			{}
	};

namespace custom_mbox_details {

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

//
// creator_iface_t
//
/*!
 * \brief Interface for creator of new mbox in OOP style.
 *
 * This interface is necessary to hide details of creation of custom mbox.
 *
 * \since
 * v.5.5.19.2
 */
class creator_iface_t
	{
	protected :
		// Non-virtual destructor to prevent creation of dynamically
		// allocated objects of that type.
		~creator_iface_t()
			{}

	public :
		//! Creation of custom mbox.
		virtual mbox_t
		create(
			//! An environment-specific data to be used for new mbox.
			const mbox_creation_data_t & data ) = 0;
	};

//
// creator_template_t
//
/*!
 * \brief Template-based implementation of creator interface.
 *
 * \tparam LAMBDA type of actual lambda with all creation actions.
 *
 * \since
 * v.5.5.19.2
 */
template< typename LAMBDA >
class creator_template_t : public creator_iface_t 
	{
		//! Lambda or functional object to be used for custom mbox creation.
		LAMBDA m_lambda;

	public :
		creator_template_t( LAMBDA && lambda ) : m_lambda(std::move(lambda))
			{}
		~creator_template_t()
			{}

		virtual mbox_t
		create(
			const mbox_creation_data_t & data ) override
			{
				return m_lambda( data );
			}
	};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} /* namespace custom_mbox_details */

} /* namespace so_5 */

