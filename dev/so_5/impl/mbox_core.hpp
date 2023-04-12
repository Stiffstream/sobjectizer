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

#include <so_5/mbox.hpp>
#include <so_5/mchain.hpp>
#include <so_5/nonempty_name.hpp>

#include <so_5/message_limit.hpp>

#include <so_5/atomic_refcounted.hpp>
#include <so_5/msg_tracing.hpp>
#include <so_5/outliving.hpp>

#include <so_5/custom_mbox.hpp>

namespace so_5
{

namespace impl
{

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
class mbox_core_t final : private atomic_refcounted_t
{
		friend class intrusive_ptr_t< mbox_core_t >;

		mbox_core_t( const mbox_core_t & ) = delete;
		mbox_core_t & operator=( const mbox_core_t & ) = delete;

	public:
		mbox_core_t(
			//! Message delivery tracing stuff.
			outliving_reference_t< so_5::msg_tracing::holder_t > msg_tracing_stuff );

		//! Create local anonymous mbox.
		/*!
			\note always creates a new mbox.
		*/
		[[nodiscard]]
		mbox_t
		create_mbox( environment_t & env );

		//! Create local named mbox.
		/*!
			\note if mbox with specified name \a mbox_name is present, 
			method won't create a new mbox. In this case method 
			will return a new mbox_t, which links to 
			the present mbox (with this name).
		*/
		[[nodiscard]]
		mbox_t
		create_mbox(
			//! Environment for which the mbox is created.
			environment_t & env,
			//! Mbox name.
			nonempty_name_t mbox_name );

		/*!
		 * \brief Create mpsc_mbox that handles message limits.
		 *
		 * \since v.5.8.0
		 */
		[[nodiscard]]
		mbox_t
		create_ordinary_mpsc_mbox(
			//! The only consumer for the mbox.
			agent_t & owner );

		/*!
		 * \brief Create mpsc_mbox that ignores message limits.
		 *
		 * \since v.5.8.0
		 */
		[[nodiscard]]
		mbox_t
		create_limitless_mpsc_mbox(
			//! The only consumer for the mbox.
			agent_t & owner );

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
			//! Environment for which the mbox is created.
			environment_t & env,
			//! Creator for new mbox.
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

		/*!
		 * \brief Allocate an ID for a new custom mbox or mchain.
		 *
		 * \since
		 * v.5.7.0
		 */
		[[nodiscard]] mbox_id_t
		allocate_mbox_id() noexcept;

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
			named_mbox_info_t( mbox_t mbox )
				:
					m_external_ref_count( 1 ),
					m_mbox( std::move(mbox) )
			{}

			//! Reference count by external mbox_refs.
			unsigned int m_external_ref_count;
			//! Real mbox for that name.
			mbox_t m_mbox;
		};

		//! Typedef for the map from the mbox name to the mbox information.
		using named_mboxes_dictionary_t = std::map<
				std::string,
				named_mbox_info_t >;

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
/*!
 * \note
 * It was a separate class until v.5.8.0.
 * Since v.5.8.0 it's just a typedef for intrusive_ptr_t.
 */
using mbox_core_ref_t = intrusive_ptr_t< mbox_core_t >;

} /* namespace impl */

} /* namespace so_5 */

