/*
	SObjectizer 5.
*/

#include <so_5/impl/mbox_core.hpp>

#include <so_5/exception.hpp>

#include <so_5/impl/local_mbox.hpp>
#include <so_5/impl/named_local_mbox.hpp>
#include <so_5/impl/mpsc_mbox.hpp>
#include <so_5/impl/mchain_details.hpp>
#include <so_5/impl/make_mchain.hpp>

#include <algorithm>

namespace so_5
{

namespace impl
{

//
// mbox_core_t
//

mbox_core_t::mbox_core_t(
	outliving_reference_t< so_5::msg_tracing::holder_t > msg_tracing_stuff )
	:	m_msg_tracing_stuff{ msg_tracing_stuff }
	,	m_mbox_id_counter{ 1 }
{
}

mbox_t
mbox_core_t::create_mbox(
	environment_t & env )
{
	auto id = ++m_mbox_id_counter;
	if( !m_msg_tracing_stuff.get().is_msg_tracing_enabled() )
		return mbox_t{ new local_mbox_without_tracing{ id, env } };
	else
		return mbox_t{ new local_mbox_with_tracing{ id, env, m_msg_tracing_stuff } };
}

mbox_t
mbox_core_t::create_mbox(
	environment_t & env,
	nonempty_name_t mbox_name )
{
	mbox_t result; // Will be created later,

	full_named_mbox_id_t key{
			default_global_mbox_namespace(),
			mbox_name.giveout_value()
		};
	// NOTE: mbox_name can't be used anymore!

	std::lock_guard< std::mutex > lock( m_dictionary_lock );

	named_mboxes_dictionary_t::iterator it =
		m_named_mboxes_dictionary.find( key );

	if( m_named_mboxes_dictionary.end() != it )
	{
		// For strong exception safety create a new instance
		// of named_local_mbox first...
		result = mbox_t{
				new named_local_mbox_t( key, it->second.m_mbox, *this )
			};

		// ... now the count of references can be incremented safely
		// (exceptions is no more expected).
		++(it->second.m_external_ref_count);
	}
	else
	{
		// There is no mbox with such name. New mbox should be created.
		// NOTE: it's safe to call create_mbox(env) when mbox_core is
		// locked because create_mbox(env) doesn't to lock the mbox_core.
		mbox_t mbox_ref = create_mbox( env );

		// For strong exception safety create a new instance
		// of named_local_mbox first...
		result = mbox_t{
				new named_local_mbox_t( key, mbox_ref, *this )
			};

		// ...now we can update the dictionary. If there will be an exception
		// then all new object will be destroyed automatically.
		m_named_mboxes_dictionary.emplace(
				key,
				named_mbox_info_t( mbox_ref ) );
	}

	return result;
}

namespace {

template< typename M1, typename M2, typename... A >
std::unique_ptr< abstract_message_box_t >
make_actual_mbox(
	outliving_reference_t<so_5::msg_tracing::holder_t> msg_tracing_stuff,
	A &&... args )
	{
		std::unique_ptr< abstract_message_box_t > result;

		if( !msg_tracing_stuff.get().is_msg_tracing_enabled() )
			result.reset( new M1{ std::forward<A>(args)... } );
		else
			result.reset(
					new M2{ std::forward<A>(args)..., msg_tracing_stuff.get() } );

		return result;
	}

} /* namespace anonymous */

mbox_t
mbox_core_t::create_ordinary_mpsc_mbox(
	environment_t & env,
	agent_t & owner )
{
	const auto id = ++m_mbox_id_counter;

	std::unique_ptr< abstract_message_box_t > actual_mbox =
			make_actual_mbox<
							ordinary_mpsc_mbox_without_tracing_t,
							ordinary_mpsc_mbox_with_tracing_t >(
					m_msg_tracing_stuff,
					id,
					env,
					outliving_mutable( owner ) );

	return mbox_t{ actual_mbox.release() };
}

mbox_t
mbox_core_t::create_limitless_mpsc_mbox(
	environment_t & env,
	agent_t & owner )
{
	const auto id = ++m_mbox_id_counter;

	std::unique_ptr< abstract_message_box_t > actual_mbox =
			make_actual_mbox<
							limitless_mpsc_mbox_without_tracing_t,
							limitless_mpsc_mbox_with_tracing_t >(
					m_msg_tracing_stuff,
					id,
					env,
					outliving_mutable( owner ) );

	return mbox_t{ actual_mbox.release() };
}

void
mbox_core_t::destroy_mbox(
	const full_named_mbox_id_t & name ) noexcept
{
	std::lock_guard< std::mutex > lock( m_dictionary_lock );

	named_mboxes_dictionary_t::iterator it =
		m_named_mboxes_dictionary.find( name );

	if( m_named_mboxes_dictionary.end() != it )
	{
		const unsigned int ref_count = --(it->second.m_external_ref_count);
		if( 0 == ref_count )
			m_named_mboxes_dictionary.erase( it );
	}
}

mbox_t
mbox_core_t::create_custom_mbox(
	environment_t & env,
	::so_5::custom_mbox_details::creator_iface_t & creator )
{
	const auto id = ++m_mbox_id_counter;
	return creator.create(
			mbox_creation_data_t{
					outliving_mutable(env),
					id,
					outliving_mutable(m_msg_tracing_stuff)
			} );
}

mbox_t
mbox_core_t::introduce_named_mbox(
	mbox_namespace_name_t mbox_namespace,
	nonempty_name_t mbox_name,
	const std::function< mbox_t() > & mbox_factory )
{
	mbox_t result;

	full_named_mbox_id_t key{
			std::string{ mbox_namespace.query_name() },
			mbox_name.giveout_value()
		};
	// NOTE: mbox_name can't be used anymore!

	// Step 1. Check the presense of this mbox.
	// It's important to do that step on locked object.
	{
		std::lock_guard< std::mutex > lock( m_dictionary_lock );

		named_mboxes_dictionary_t::iterator it =
			m_named_mboxes_dictionary.find( key );

		if( m_named_mboxes_dictionary.end() != it )
		{
			// For strong exception safety create a new instance
			// of named_local_mbox first...
			result = mbox_t{
					new named_local_mbox_t( key, it->second.m_mbox, *this )
				};

			// ... now the count of references can be incremented safely
			// (exceptions is no more expected).
			++(it->second.m_external_ref_count);
		}
	}

	if( !result )
	{
		// Step 2. Create a new instance on mbox.
		// It's important to call mbox_factory when mbox_core isn't locked.
		auto fresh_mbox = mbox_factory();
		if( !fresh_mbox )
			SO_5_THROW_EXCEPTION(
					rc_nullptr_as_result_of_user_mbox_factory,
					"user-provided mbox_factory returns nullptr" );

		// Step 3. Try to register the fresh_mbox.
		// It has to be done on locked object.
		{
			std::lock_guard< std::mutex > lock( m_dictionary_lock );

			// Another search. This is necessary because the name may have been
			// created while mbox_factory() was running.
			named_mboxes_dictionary_t::iterator it =
				m_named_mboxes_dictionary.find( key );

			if( m_named_mboxes_dictionary.end() != it )
			{
				// Yes, the name has been created while we were inside
				// mbox_factory() call. The fresh_mbox has to be discarded.
				//
				// For strong exception safety create a new instance
				// of named_local_mbox first...
				result = mbox_t{
						new named_local_mbox_t( key, it->second.m_mbox, *this )
					};

				// ... now the count of references can be incremented safely
				// (exceptions is no more expected).
				++(it->second.m_external_ref_count);
			}
			else
			{
				// For strong exception safety create a new instance
				// of named_local_mbox first...
				result = mbox_t{
						new named_local_mbox_t( key, fresh_mbox, *this )
					};

				// ...now we can update the dictionary. If there will be an
				// exception then all new object will be destroyed automatically.
				m_named_mboxes_dictionary.emplace(
						key,
						named_mbox_info_t( fresh_mbox ) );
			}
		}
	}

	return result;
}

mchain_t
mbox_core_t::create_mchain(
	environment_t & env,
	const mchain_params_t & params )
{
	using namespace so_5::mchain_props;
	using namespace so_5::mchain_props::details;

	auto id = ++m_mbox_id_counter;

	if( params.capacity().unlimited() )
		return make_mchain< unlimited_demand_queue >(
				m_msg_tracing_stuff, params, env, id );
	else if( memory_usage_t::dynamic == params.capacity().memory_usage() )
		return make_mchain< limited_dynamic_demand_queue >(
				m_msg_tracing_stuff, params, env, id );
	else
		return make_mchain< limited_preallocated_demand_queue >(
				m_msg_tracing_stuff, params, env, id );
}

mbox_core_stats_t
mbox_core_t::query_stats()
{
	std::lock_guard< std::mutex > lock{ m_dictionary_lock };

	return mbox_core_stats_t{ m_named_mboxes_dictionary.size() };
}

[[nodiscard]] mbox_id_t
mbox_core_t::allocate_mbox_id() noexcept
{
	return ++m_mbox_id_counter;
}

} /* namespace impl */

} /* namespace so_5 */
