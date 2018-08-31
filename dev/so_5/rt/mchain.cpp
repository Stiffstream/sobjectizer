/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.13
 *
 * \file
 * \brief Public part of message chain related stuff.
 */

#include <so_5/rt/h/mchain.hpp>

namespace so_5 {

//
// abstract_message_chain_t
//
mbox_t
abstract_message_chain_t::as_mbox()
	{
		return mbox_t{ this };
	}

mchain_props::extraction_status_t
abstract_message_chain_t::extract(
	mchain_props::demand_t & /*dest*/,
	mchain_props::select_case_t & /*select_case*/ )
	{
		SO_5_THROW_EXCEPTION( rc_not_implemented,
				"abstract_message_chain_t::extract(demand,select_case) is "
				"not implemented in abstract_message_chain_t class" );

		// Just to make compilers happy.
		return mchain_props::extraction_status_t::no_messages;
	}

void
abstract_message_chain_t::remove_from_select(
	mchain_props::select_case_t & /*select_case*/ )
	{
		SO_5_THROW_EXCEPTION( rc_not_implemented,
				"abstract_message_chain_t::remove_from_select(select_case) is "
				"not implemented in abstract_message_chain_t class" );
	}

} /* namespace so_5 */

