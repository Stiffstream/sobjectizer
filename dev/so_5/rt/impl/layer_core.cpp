/*
	SObjectizer 5.
*/

#include <algorithm>
#include <iterator>

#include <so_5/rt/impl/h/layer_core.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

//
// typed_layer_ref_t
//

typed_layer_ref_t::typed_layer_ref_t()
	:
		m_true_type( std::type_index( typeid( int ) ) )
{
}

typed_layer_ref_t::typed_layer_ref_t(
	const so_layer_map_t::value_type & v )
	:
		m_true_type( v.first ),
		m_layer( v.second )
{
}

typed_layer_ref_t::typed_layer_ref_t(
	const std::type_index & type,
	const so_layer_ref_t & layer )
	:
		m_true_type( type ),
		m_layer( layer )
{
}

bool
typed_layer_ref_t::operator < ( const typed_layer_ref_t & tl ) const
{
	return m_true_type < tl.m_true_type;
}

//
// layer_core_t
//

layer_core_t::layer_core_t(
	environment_t & env,
	const so_layer_map_t & so_layers )
	:
		m_env( env ),
		m_default_layers( so_layers.begin(), so_layers.end() )
{
	std::for_each( m_default_layers.begin(), m_default_layers.end(),
		[this]( so_layer_list_t::value_type & item )
		{
			item.m_layer->bind_to_environment( &m_env );
		} );
}

layer_core_t::~layer_core_t()
{
}

//! Find layer in container.
inline so_layer_list_t::const_iterator
search_for_layer(
	const so_layer_list_t & layers,
	const std::type_index & type )
{
	auto search_result = std::lower_bound( layers.begin(), layers.end(),
			type,
			[]( const so_layer_list_t::value_type & a, const std::type_index & b )
			{
				return a.m_true_type < b;
			} );
	if( search_result != layers.end() && search_result->m_true_type == type )
		return search_result;

	return layers.end();
}

so_layer_t *
layer_core_t::query_layer(
	const std::type_index & type ) const
{
	// Try search within default layers first.
	so_layer_list_t::const_iterator layer_it = search_for_layer(
		m_default_layers,
		type );

	if( m_default_layers.end() != layer_it )
		return layer_it->m_layer.get();

	// Layer not found yet. Search extra layers.
	std::lock_guard< std::mutex > lock( m_extra_layers_lock );

	layer_it = search_for_layer(
		m_extra_layers,
		type );

	if( m_extra_layers.end() != layer_it )
		return layer_it->m_layer.get();

	return nullptr;
}

void
layer_core_t::start()
{
	so_layer_list_t::iterator
		it = m_default_layers.begin(),
		it_end = m_default_layers.end();

	for(; it != it_end; ++it )
	{
		try
		{
			it->m_layer->start();
		}
		catch( const std::exception & )
		{
			so_layer_list_t::iterator it_stoper = m_default_layers.begin();
			for(; it_stoper != it; ++it_stoper )
				it_stoper->m_layer->shutdown();

			for(it_stoper = m_default_layers.begin();
					it_stoper != it; ++it_stoper )
				it_stoper->m_layer->wait();

			throw;
		}
	}
}

void
layer_core_t::finish()
{
	// Shutdown and wait extra layers.
	shutdown_extra_layers();
	wait_extra_layers();

	// Shutdown and wait default layers.
	shutdown_default_layers();
	wait_default_layers();
}

void
layer_core_t::add_extra_layer(
	const std::type_index & type,
	const so_layer_ref_t & layer )
{
	if( nullptr == layer.get() )
		SO_5_THROW_EXCEPTION(
			rc_trying_to_add_nullptr_extra_layer,
			"trying to add nullptr extra layer" );

	if( m_default_layers.end() != search_for_layer( m_default_layers, type ) )
		SO_5_THROW_EXCEPTION(
			rc_trying_to_add_extra_layer_that_already_exists_in_default_list,
			"trying to add extra layer that already exists in default list" );

	std::lock_guard< std::mutex > lock( m_extra_layers_lock );

	if( m_extra_layers.end() != search_for_layer( m_extra_layers, type ) )
		SO_5_THROW_EXCEPTION(
			rc_trying_to_add_extra_layer_that_already_exists_in_extra_list,
			"trying to add extra layer that already exists in extra list" );

	layer->bind_to_environment( &m_env );

	try
	{
		layer->start();
	}
	catch( const std::exception & x )
	{
		SO_5_THROW_EXCEPTION(
			rc_unable_to_start_extra_layer,
			std::string( "layer raised an exception: " ) + x.what() );
	}

	try
	{
		typed_layer_ref_t typed_layer( type, layer );

		m_extra_layers.insert(
			std::lower_bound(
				m_extra_layers.begin(),
				m_extra_layers.end(),
				typed_layer ),
			typed_layer );
	}
	catch( const std::exception & x )
	{
		// Unable to store layer. Layer must be stopped...
		layer->shutdown();
		layer->wait();

		// ...And error must be reported.
		SO_5_THROW_EXCEPTION(
			rc_unable_to_start_extra_layer,
			std::string( "unable to store pointer to layer, exception: " ) +
				x.what() );
	}
}

void
call_shutdown( typed_layer_ref_t &  tl )
{
	tl.m_layer->shutdown();
}
void
call_wait( typed_layer_ref_t &  tl )
{
	tl.m_layer->wait();
}

void
layer_core_t::shutdown_extra_layers()
{
	std::for_each(
		m_extra_layers.begin(),
		m_extra_layers.end(),
		call_shutdown );
}

void
layer_core_t::wait_extra_layers()
{
	std::for_each(
		m_extra_layers.begin(),
		m_extra_layers.end(),
		call_wait );

	m_extra_layers.clear();
}

void
layer_core_t::shutdown_default_layers()
{
	std::for_each(
		m_default_layers.begin(),
		m_default_layers.end(),
		call_shutdown );
}

void
layer_core_t::wait_default_layers()
{
	std::for_each(
		m_default_layers.begin(),
		m_default_layers.end(),
		call_wait );

}

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */
