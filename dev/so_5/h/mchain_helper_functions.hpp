/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Helper functions for mchains.
 * \since
 * v.5.5.16
 */

#pragma once

#include <so_5/rt/h/mchain.hpp>

#include <so_5/h/wrapped_env.hpp>

#include <array>

namespace so_5 {

/*!
 * \name Helper functions for mchain creation.
 * \{
 */

/*!
 * \brief Create size-unlimited chain.
 *
 * \code
	so_5::environment_t & env = ...;
	auto ch = create_mchain( env );

	// Or as more verbose alternative:
	auto ch = env.create_mchain( so_5::make_unlimited_mchain_params() );
 * \endcode
 * 
 * \since
 * v.5.5.16
 */
inline mchain_t
create_mchain( environment_t & env )
	{
		return env.create_mchain( make_unlimited_mchain_params() );
	}

/*!
 * \brief Create size-unlimited chain.
 *
 * \code
	wrapped_env_t sobj;
	auto ch = create_mchain( sobj );

	// Or as more verbose alternative:
	auto ch = sobj.environment().create_mchain( make_unlimited_mchain_params() );
 * \endcode
 * 
 * \since
 * v.5.5.16
 */
inline mchain_t
create_mchain( wrapped_env_t & sobj )
	{
		return sobj.environment().create_mchain( so_5::make_unlimited_mchain_params() );
	}

/*!
 * \brief Create size-limited chain without waiting on overflow.
 *
 * \code
	so_5::environment_t & env = ...;
	auto ch = create_mchain( env,
			// No more than 200 messages in the chain.
			200,
			// Memory will be allocated dynamically.
			so_5::mchain_props::memory_usage_t::dynamic,
			// New messages will be ignored on chain's overflow.
			so_5::mchain_props::overflow_reaction_t::drop_newest );

	// Or as more verbose alternative:
	auto ch = env.create_mchain( so_5::make_limited_without_waiting_mchain_params(
			// No more than 200 messages in the chain.
			200,
			// Memory will be allocated dynamically.
			so_5::mchain_props::memory_usage_t::dynamic,
			// New messages will be ignored on chain's overflow.
			so_5::mchain_props::overflow_reaction_t::drop_newest ) );
 * \endcode
 *
 * \since
 * v.5.5.16
 */
inline mchain_t
create_mchain(
	//! Environment for mchain.
	environment_t & env,
	//! Max capacity of mchain.
	std::size_t max_size,
	//! Type of chain storage.
	mchain_props::memory_usage_t memory_usage,
	//! Reaction on chain overflow.
	mchain_props::overflow_reaction_t overflow_reaction )
	{
		return env.create_mchain(
				make_limited_without_waiting_mchain_params(
						max_size, memory_usage, overflow_reaction ) );
	}

/*!
 * \brief Create size-limited chain without waiting on overflow.
 *
 * \code
	so_5::wrapped_env_t sobj;
	auto ch = create_mchain( sobj,
			// No more than 200 messages in the chain.
			200,
			// Memory will be allocated dynamically.
			so_5::mchain_props::memory_usage_t::dynamic,
			// New messages will be ignored on chain's overflow.
			so_5::mchain_props::overflow_reaction_t::drop_newest );

	// Or as more verbose alternative:
	auto ch = sobj.environment().create_mchain(
			so_5::make_limited_without_waiting_mchain_params(
				// No more than 200 messages in the chain.
				200,
				// Memory will be allocated dynamically.
				so_5::mchain_props::memory_usage_t::dynamic,
				// New messages will be ignored on chain's overflow.
				so_5::mchain_props::overflow_reaction_t::drop_newest ) );
 * \endcode
 *
 * \since
 * v.5.5.16
 */
inline mchain_t
create_mchain(
	//! Environment for mchain.
	wrapped_env_t & sobj,
	//! Max capacity of mchain.
	std::size_t max_size,
	//! Type of chain storage.
	mchain_props::memory_usage_t memory_usage,
	//! Reaction on chain overflow.
	mchain_props::overflow_reaction_t overflow_reaction )
	{
		return sobj.environment().create_mchain(
				make_limited_without_waiting_mchain_params(
						max_size, memory_usage, overflow_reaction ) );
	}
/*!
 * \}
 */

/*!
 * \brief Create size-limited chain with waiting on overflow.
 *
 * \code
	so_5::environment_t & env = ...;
	auto ch = create_mchain( env,
			// Wait for 150ms.
			std::chrono::milliseconds{150},
			// No more than 200 messages in the chain.
			200,
			// Memory will be allocated dynamically.
			so_5::mchain_props::memory_usage_t::dynamic,
			// New messages will be ignored on chain's overflow.
			so_5::mchain_props::overflow_reaction_t::drop_newest );

	// Or as more verbose alternative:
	auto ch = env.create_mchain( so_5::make_limited_with_waiting_mchain_params(
			// No more than 200 messages in the chain.
			200,
			// Memory will be allocated dynamically.
			so_5::mchain_props::memory_usage_t::dynamic,
			// New messages will be ignored on chain's overflow.
			so_5::mchain_props::overflow_reaction_t::drop_newest,
			// Wait for 150ms.
			std::chrono::milliseconds{150} ) );
 * \endcode
 *
 * \since
 * v.5.5.16
 */
inline mchain_t
create_mchain(
	//! Environment for mchain.
	environment_t & env,
	//! Max waiting time in case of overflow.
	mchain_props::duration_t waiting_time,
	//! Max capacity of mchain.
	std::size_t max_size,
	//! Type of chain storage.
	mchain_props::memory_usage_t memory_usage,
	//! Reaction on chain overflow.
	mchain_props::overflow_reaction_t overflow_reaction )
	{
		return env.create_mchain(
				make_limited_with_waiting_mchain_params(
						max_size, memory_usage, overflow_reaction, waiting_time ) );
	}

/*!
 * \brief Create size-limited chain without waiting on overflow.
 *
 * \code
	so_5::wrapped_env_t sobj;
	auto ch = create_mchain( sobj,
			// Wait for 150ms.
			std::chrono::milliseconds{150},
			// No more than 200 messages in the chain.
			200,
			// Memory will be allocated dynamically.
			so_5::mchain_props::memory_usage_t::dynamic,
			// New messages will be ignored on chain's overflow.
			so_5::mchain_props::overflow_reaction_t::drop_newest );

	// Or as more verbose alternative:
	auto ch = sobj.environment().create_mchain(
			so_5::make_limited_with_waiting_mchain_params(
				// No more than 200 messages in the chain.
				200,
				// Memory will be allocated dynamically.
				so_5::mchain_props::memory_usage_t::dynamic,
				// New messages will be ignored on chain's overflow.
				so_5::mchain_props::overflow_reaction_t::drop_newest,
				// Wait for 150ms.
				std::chrono::milliseconds{150} ) );
 * \endcode
 *
 * \since
 * v.5.5.16
 */
inline mchain_t
create_mchain(
	//! Environment for mchain.
	wrapped_env_t & sobj,
	//! Max waiting time in case of overflow.
	mchain_props::duration_t waiting_time,
	//! Max capacity of mchain.
	std::size_t max_size,
	//! Type of chain storage.
	mchain_props::memory_usage_t memory_usage,
	//! Reaction on chain overflow.
	mchain_props::overflow_reaction_t overflow_reaction )
	{
		return sobj.environment().create_mchain(
				make_limited_with_waiting_mchain_params(
						max_size, memory_usage, overflow_reaction, waiting_time ) );
	}
/*!
 * \}
 */

namespace mchain_auto_close_details {

/*!
 * \brief Main class for auto close operation.
 *
 * \attention This is Moveable class, not Copyable.
 *
 * \since
 * v.5.5.16
 */
template< std::size_t N >
class auto_closer_t
	{
		std::array< mchain_t, N > m_chains;
		so_5::mchain_props::close_mode_t m_close_mode;

	public :
		auto_closer_t( const auto_closer_t & ) = delete;
		auto_closer_t & operator=( const auto_closer_t & ) = delete;

		auto_closer_t(
			so_5::mchain_props::close_mode_t close_mode )
			:	m_close_mode{ close_mode }
			{}

		auto_closer_t( auto_closer_t && o )
			:	m_close_mode{ o.m_close_mode }
			{
				m_chains.swap( o.m_chains );
			}

		~auto_closer_t()
			{
				for( auto & ch : m_chains )
					if( ch )
						ch->close( m_close_mode );
			}

		friend inline void
		swap( auto_closer_t & a, auto_closer_t & b ) SO_5_NOEXCEPT
			{
				std::swap( a.m_close_mode, b.m_close_mode );
				a.m_chains.swap( b.m_chains );
			}

		auto_closer_t &
		operator=( auto_closer_t && o )
			{
				auto_closer_t tmp{ std::move(o) };
				swap( *this, tmp );
				return *this;
			}

		void
		set_mchain( std::size_t index, mchain_t ch )
			{
				m_chains[ index ] = std::move(ch);
			}
	};

/*!
 * \brief Helper for filling auto_closer object.
 *
 * \since
 * v.5.5.16
 */
template< typename CLOSER >
void
add_to_closer( CLOSER & to, std::size_t index, mchain_t ch )
	{
		to.set_mchain( index, std::move(ch) );
	}

/*!
 * \brief Helper for filling auto_closer object.
 *
 * \since
 * v.5.5.16
 */
template< typename CLOSER, typename... TAIL >
void
add_to_closer(
	CLOSER & to,
	std::size_t index,
	mchain_t ch,
	TAIL &&... tail )
	{
		add_to_closer( to, index, std::move(ch) );
		add_to_closer( to, index + 1, std::forward< TAIL >(tail)... );
	}

} /* namespace mchain_auto_close_details */

/*!
 * \brief Helper function for creation of automatic closer of mchains.
 *
 * Usage examples:
 * \code
	so_5::environment_t & env = ...;
	auto command_ch = create_mchain( env );
	auto reply_ch = create_mchain( env );
	std::thread worker{ [command_ch, reply_ch] {
			receive( from( command_ch ), handlers... );
		} };
	// Chains must be closed automatically on any exception...
	// Content of chains must be dropped.
	auto channels_closer = so_5::auto_close_mchains(
			so_5::mchain_props::close_mode_t::drop_content,
			command_ch,
			reply_ch );

	... // Some complex code here.

	// Close command chain for worker thread. Worker will finish its work.
	close_retain_content( command_ch );
	// Ensure that worker finished.
	worker.join();
 * \endcode
 *
 * \since
 * v.5.5.16
 */
template< typename... TAIL >
mchain_auto_close_details::auto_closer_t< sizeof...(TAIL) >
auto_close_mchains(
	//! Close mode for all mchains.
	//! This value will be passed to all close() calls.
	mchain_props::close_mode_t close_mode,
	//! Chains to be closed.
	TAIL &&... tail )
	{
		using namespace mchain_auto_close_details;

		auto_closer_t< sizeof...(tail) > closer{ close_mode };
		add_to_closer( closer, 0, std::forward< TAIL >(tail)... );

		return closer;
	}

} /* namespace so_5 */

