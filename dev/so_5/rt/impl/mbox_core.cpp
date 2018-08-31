/*
	SObjectizer 5.
*/

#include <so_5/rt/impl/h/mbox_core.hpp>

#include <so_5/h/exception.hpp>

#include <so_5/rt/impl/h/local_mbox.hpp>
#include <so_5/rt/impl/h/named_local_mbox.hpp>
#include <so_5/rt/impl/h/mpsc_mbox.hpp>
#include <so_5/rt/impl/h/mchain_details.hpp>

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
mbox_core_t::create_mbox()
{
	auto id = ++m_mbox_id_counter;
	if( !m_msg_tracing_stuff.get().is_msg_tracing_enabled() )
		return mbox_t{ new local_mbox_without_tracing{ id } };
	else
		return mbox_t{ new local_mbox_with_tracing{ id, m_msg_tracing_stuff } };
}

mbox_t
mbox_core_t::create_mbox(
	nonempty_name_t mbox_name )
{
	return create_named_mbox(
			std::move(mbox_name),
			[this]() { return create_mbox(); } );
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
					new M2{ std::forward<A>(args)...,
							msg_tracing_stuff.get() } );

		return result;
	}

} /* namespace anonymous */

mbox_t
mbox_core_t::create_mpsc_mbox(
	agent_t * single_consumer,
	const so_5::message_limit::impl::info_storage_t * limits_storage )
{
	const auto id = ++m_mbox_id_counter;

	std::unique_ptr< abstract_message_box_t > actual_mbox;
	if( limits_storage )
	{
		actual_mbox =
				make_actual_mbox<
						limitful_mpsc_mbox_without_tracing,
						limitful_mpsc_mbox_with_tracing >(
					m_msg_tracing_stuff,
					id,
					single_consumer,
					*limits_storage );
	}
	else
	{
		actual_mbox =
				make_actual_mbox<
						limitless_mpsc_mbox_without_tracing,
						limitless_mpsc_mbox_with_tracing >(
					m_msg_tracing_stuff,
					id,
					single_consumer );
	}

	return mbox_t{ actual_mbox.release() };
}

void
mbox_core_t::destroy_mbox(
	const std::string & name )
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
	::so_5::custom_mbox_details::creator_iface_t & creator )
{
	const auto id = ++m_mbox_id_counter;
	return creator.create(
			mbox_creation_data_t( id, m_msg_tracing_stuff ) );
}

namespace {

template< typename Q, typename... A >
mchain_t
make_mchain(
	outliving_reference_t< so_5::msg_tracing::holder_t > tracer,
	const mchain_params_t & params,
	A &&... args )
	{
		using namespace so_5::mchain_props;
		using namespace so_5::impl::msg_tracing_helpers;
		using D = mchain_tracing_disabled_base;
		using E = mchain_tracing_enabled_base;

		if( tracer.get().is_msg_tracing_enabled()
				&& !params.msg_tracing_disabled() )
			return mchain_t{
					new mchain_template< Q, E >{
						std::forward<A>(args)...,
						params,
						tracer } };
		else
			return mchain_t{
					new mchain_template< Q, D >{
						std::forward<A>(args)..., params } };
	}

} /* namespace anonymous */

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

mbox_t
mbox_core_t::create_named_mbox(
	nonempty_name_t nonempty_name,
	const std::function< mbox_t() > & factory )
{
	const std::string & name = nonempty_name.query_name();
	std::lock_guard< std::mutex > lock( m_dictionary_lock );

	named_mboxes_dictionary_t::iterator it =
		m_named_mboxes_dictionary.find( name );

	if( m_named_mboxes_dictionary.end() != it )
	{
		++(it->second.m_external_ref_count);
		return mbox_t(
			new named_local_mbox_t(
				name,
				it->second.m_mbox,
				*this ) );
	}

	// There is no mbox with such name. New mbox should be created.
	mbox_t mbox_ref = factory();

	m_named_mboxes_dictionary[ name ] = named_mbox_info_t( mbox_ref );

	return mbox_t( new named_local_mbox_t( name, mbox_ref, *this ) );
}

//
// mbox_core_ref_t
//

mbox_core_ref_t::mbox_core_ref_t()
	:
		m_mbox_core_ptr( nullptr )
{
}

mbox_core_ref_t::mbox_core_ref_t(
	mbox_core_t * mbox_core )
	:
		m_mbox_core_ptr( mbox_core )
{
	inc_mbox_core_ref_count();
}

mbox_core_ref_t::mbox_core_ref_t(
	const mbox_core_ref_t & mbox_core_ref )
	:
		m_mbox_core_ptr( mbox_core_ref.m_mbox_core_ptr )
{
	inc_mbox_core_ref_count();
}

void
mbox_core_ref_t::operator = (
	const mbox_core_ref_t & mbox_core_ref )
{
	if( &mbox_core_ref != this )
	{
		dec_mbox_core_ref_count();

		m_mbox_core_ptr = mbox_core_ref.m_mbox_core_ptr;
		inc_mbox_core_ref_count();
	}

}

mbox_core_ref_t::~mbox_core_ref_t()
{
	dec_mbox_core_ref_count();
}

inline void
mbox_core_ref_t::dec_mbox_core_ref_count()
{
	if( m_mbox_core_ptr &&
		0 == m_mbox_core_ptr->dec_ref_count() )
	{
		delete m_mbox_core_ptr;
		m_mbox_core_ptr = nullptr;
	}
}

inline void
mbox_core_ref_t::inc_mbox_core_ref_count()
{
	if( m_mbox_core_ptr )
		m_mbox_core_ptr->inc_ref_count();
}

} /* namespace impl */

} /* namespace so_5 */
