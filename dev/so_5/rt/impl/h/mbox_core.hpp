/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A definition of an utility class for work with mboxes.
*/

#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <mutex>

#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/mchain.hpp>
#include <so_5/rt/h/nonempty_name.hpp>

#include <so_5/rt/h/message_limit.hpp>

#include <so_5/h/atomic_refcounted.hpp>
#include <so_5/h/msg_tracing.hpp>
#include <so_5/h/outliving.hpp>

#include <so_5/h/custom_mbox.hpp>

namespace so_5
{

class environment_t;

namespace impl
{

class mbox_core_ref_t;

//
// mbox_core_stats_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Statistics from mbox_core for run-time monitoring.
 */
struct mbox_core_stats_t
	{
		//! Count of named mboxes.
		std::size_t m_named_mbox_count;
	};

//
// mbox_core_t
//

/*!
 * \brief A utility class for the work with mboxes.
 */
class mbox_core_t
	:
		private atomic_refcounted_t
{
		friend class mbox_core_ref_t;

		mbox_core_t( const mbox_core_t & );
		void
		operator = ( const mbox_core_t & );

	public:
		mbox_core_t(
			//! Message delivery tracing stuff.
			outliving_reference_t< so_5::msg_tracing::holder_t > msg_tracing_stuff );

		//! Create local anonymous mbox.
		/*!
			\note always creates a new mbox.
		*/
		mbox_t
		create_mbox();

		//! Create local named mbox.
		/*!
			\note if mbox with specified name \a mbox_name is present, 
			method won't create a new mbox. In this case method 
			will return a new mbox_t, which links to 
			the present mbox (with this name).
		*/
		mbox_t
		create_mbox(
			//! Mbox name.
			nonempty_name_t mbox_name );

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Create anonymous mpsc_mbox.
		 */
		mbox_t
		create_mpsc_mbox(
			//! The only consumer for messages.
			agent_t * single_consumer,
			//! Pointer to the optional message limits storage.
			//! If this pointer is null then the limitless MPSC-mbox will be
			//! created. If this pointer is not null the the MPSC-mbox with limit
			//! control will be created.
			const so_5::message_limit::impl::info_storage_t * limits_storage );

		//! Remove a reference to the named mbox.
		/*!
		 * If it was a last reference to named mbox the mbox destroyed.
		*/
		void
		destroy_mbox(
			//! Mbox name.
			const std::string & name );

		/*!
		 * \brief Create a custom mbox.
		 *
		 * \since
		 * v.5.5.19.2
		 */
		mbox_t
		create_custom_mbox(
			::so_5::custom_mbox_details::creator_iface_t & creator );

		/*!
		 * \since
		 * v.5.5.13
		 *
		 * \brief Create message chain.
		 *
		 * \par Usage examples:
		 */
		mchain_t
		create_mchain(
			//! SObjectizer Environment for which message chain will be created.
			environment_t & env,
			//! Parameters for a new chain.
			const mchain_params_t & params );

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Get statistics for run-time monitoring.
		 */
		mbox_core_stats_t
		query_stats();

	private:
		/*!
		 * \brief Data related to message delivery tracing.
		 *
		 * \since
		 * v.5.5.22
		 */
		outliving_reference_t< so_5::msg_tracing::holder_t > m_msg_tracing_stuff;

		//! Named mbox map's lock.
		std::mutex m_dictionary_lock;

		//! Named mbox information.
		struct named_mbox_info_t
		{
			named_mbox_info_t()
				:
					m_external_ref_count( 0 )
			{}

			named_mbox_info_t(
				const mbox_t mbox )
				:
					m_external_ref_count( 1 ),
					m_mbox( mbox )
			{}

			//! Reference count by external mbox_refs.
			unsigned int m_external_ref_count;
			//! Real mbox for that name.
			mbox_t m_mbox;
		};

		//! Typedef for the map from the mbox name to the mbox information.
		typedef std::map< std::string, named_mbox_info_t >
			named_mboxes_dictionary_t;

		//! Named mboxes.
		named_mboxes_dictionary_t m_named_mboxes_dictionary;

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief A counter for mbox ID generation.
		 */
		std::atomic< mbox_id_t > m_mbox_id_counter;

		/*!
		 * \since
		 * v.5.2.0
		 *
		 * \brief Low-level implementation of named mbox creation.
		 */
		mbox_t
		create_named_mbox(
			//! Mbox name.
			nonempty_name_t nonempty_name,
			//! Functional object to create new instance of mbox.
			//! Must have a prototype: mbox_t factory().
			const std::function< mbox_t() > & factory );
};

//! Smart reference to the mbox_core_t.
class mbox_core_ref_t
{
	public:
		mbox_core_ref_t();

		explicit mbox_core_ref_t(
			mbox_core_t * mbox_core );

		mbox_core_ref_t(
			const mbox_core_ref_t & mbox_core_ref );

		void
		operator = ( const mbox_core_ref_t & mbox_core_ref );

		~mbox_core_ref_t();

		inline const mbox_core_t *
		get() const
		{
			return m_mbox_core_ptr;
		}

		inline mbox_core_t *
		get()
		{
			return m_mbox_core_ptr;
		}

		inline const mbox_core_t *
		operator -> () const
		{
			return m_mbox_core_ptr;
		}

		inline mbox_core_t *
		operator -> ()
		{
			return m_mbox_core_ptr;
		}

		inline mbox_core_t &
		operator * ()
		{
			return *m_mbox_core_ptr;
		}


		inline const mbox_core_t &
		operator * () const
		{
			return *m_mbox_core_ptr;
		}

		inline bool
		operator == ( const mbox_core_ref_t & mbox_core_ref ) const
		{
			return m_mbox_core_ptr ==
				mbox_core_ref.m_mbox_core_ptr;
		}

		inline bool
		operator < ( const mbox_core_ref_t & mbox_core_ref ) const
		{
			return m_mbox_core_ptr <
				mbox_core_ref.m_mbox_core_ptr;
		}

	private:
		//! Increment reference count to the mbox_core.
		void
		inc_mbox_core_ref_count();

		//! Decrement reference count to the mbox_core.
		/*!
		 * If a reference count become 0 then mbox_core is destroyed.
		 */
		void
		dec_mbox_core_ref_count();

		mbox_core_t * m_mbox_core_ptr;
};

} /* namespace impl */

} /* namespace so_5 */
