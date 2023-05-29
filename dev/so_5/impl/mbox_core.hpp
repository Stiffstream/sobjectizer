/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A definition of an utility class for work with mboxes.
*/

#pragma once

#include <so_5/mbox.hpp>
#include <so_5/mbox_namespace_name.hpp>
#include <so_5/mchain.hpp>
#include <so_5/nonempty_name.hpp>

#include <so_5/message_limit.hpp>

#include <so_5/atomic_refcounted.hpp>
#include <so_5/msg_tracing.hpp>
#include <so_5/outliving.hpp>

#include <so_5/custom_mbox.hpp>

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

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
// full_named_mbox_id_t
//
/*!
 * \brief Full name for a named mbox.
 *
 * The full name includes mbox namespace and the name of the mbox.
 *
 * \note
 * The mbox namespace may be empty if named was created as ordinary
 * named mbox (via environment_t::create_mbox()).
 *
 * \since v.5.8.0
 */
struct full_named_mbox_id_t
	{
		/*!
		 * \brief Name of mbox namespace in that the mbox is defined.
		 *
		 * May be empty for ordinary named mbox.
		 */
		std::string m_namespace;

		/*!
		 * \brief Own name of the mbox.
		 *
		 * \attention
		 * Can't be empty.
		 */
		std::string m_name;

		//! Initializing constructor.
		full_named_mbox_id_t(
			std::string mbox_namespace,
			std::string mbox_name )
			:	m_namespace{ std::move(mbox_namespace) }
			,	m_name{ std::move(mbox_name) }
			{}
	};

[[nodiscard]]
inline bool
operator<(
	const full_named_mbox_id_t & a,
	const full_named_mbox_id_t & b )
	{
		return std::tie(a.m_namespace, a.m_name) <
				std::tie(b.m_namespace, b.m_name);
	}

//
// default_global_mbox_namespace
//
/*!
 * \brief Helper function that returns name of the default global
 * namespace for named mboxes.
 *
 * \note
 * This default global namespace has empty name in the current version
 * of SObjectizer.
 *
 * \since v.5.8.0
 */
[[nodiscard]]
inline std::string
default_global_mbox_namespace()
	{
		return {};
	}

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
			//! Environment for which the mbox is created.
			environment_t & env,
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
			//! Environment for which the mbox is created.
			environment_t & env,
			//! The only consumer for the mbox.
			agent_t & owner );

		//! Remove a reference to the named mbox.
		/*!
		 * If it was a last reference to named mbox the mbox destroyed.
		*/
		void
		destroy_mbox(
			//! Mbox name.
			const full_named_mbox_id_t & name ) noexcept;

		/*!
		 * \brief Create a custom mbox.
		 *
		 * \since
		 * v.5.5.19.2
		 */
		[[nodiscard]]
		mbox_t
		create_custom_mbox(
			//! Environment for which the mbox is created.
			environment_t & env,
			//! Creator for new mbox.
			::so_5::custom_mbox_details::creator_iface_t & creator );

		/*!
		 * \brief Introduce named mbox with user-provided factory.
		 *
		 * \since v.5.8.0
		 */
		[[nodiscard]]
		mbox_t
		introduce_named_mbox(
			//! Name of mbox_namespace for a new mbox.
			mbox_namespace_name_t mbox_namespace,
			//! Name for a new mbox.
			nonempty_name_t mbox_name,
			//! Factory for new mbox.
			const std::function< mbox_t() > & mbox_factory );

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
				full_named_mbox_id_t,
				named_mbox_info_t,
				std::less<> // It's important.
			>;

		//! Named mboxes.
		named_mboxes_dictionary_t m_named_mboxes_dictionary;

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief A counter for mbox ID generation.
		 */
		std::atomic< mbox_id_t > m_mbox_id_counter;
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

