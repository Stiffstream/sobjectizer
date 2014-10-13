/*
	SObjectizer 5.
*/

#include <algorithm>

#include <so_5/h/exception.hpp>

#include <so_5/rt/impl/h/local_mbox.hpp>
#include <so_5/rt/impl/h/named_local_mbox.hpp>
#include <so_5/rt/impl/h/mpsc_mbox.hpp>
#include <so_5/rt/impl/h/mbox_core.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

//
// mbox_core_t
//

mbox_core_t::mbox_core_t()
	:	m_mbox_id_counter( 1 )
{
}

mbox_core_t::~mbox_core_t()
{
}


mbox_t
mbox_core_t::create_local_mbox()
{
	mbox_t mbox_ref( new local_mbox_t( ++m_mbox_id_counter ) );

	return mbox_ref;
}

mbox_t
mbox_core_t::create_local_mbox(
	const nonempty_name_t & mbox_name )
{
	return create_named_mbox(
			mbox_name,
			[this]() -> mbox_t {
				return mbox_t(
					new local_mbox_t( ++m_mbox_id_counter ) );
			} );
}

mbox_t
mbox_core_t::create_mpsc_mbox(
	agent_t * single_consumer,
	event_queue_proxy_ref_t event_queue )
{
	return mbox_t(
			new mpsc_mbox_t(
					++m_mbox_id_counter,
					single_consumer,
					std::move( event_queue ) ) );
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
mbox_core_t::create_named_mbox(
	const nonempty_name_t & nonempty_name,
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

} /* namespace rt */

} /* namespace so_5 */
