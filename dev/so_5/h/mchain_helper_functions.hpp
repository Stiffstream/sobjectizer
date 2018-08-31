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
		return create_mchain( sobj.environment() );
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
template< typename Closer >
void
add_to_closer( Closer & to, std::size_t index, mchain_t ch )
	{
		to.set_mchain( index, std::move(ch) );
	}

/*!
 * \brief Helper for filling auto_closer object.
 *
 * \since
 * v.5.5.16
 */
template< typename Closer, typename... Tail >
void
add_to_closer(
	Closer & to,
	std::size_t index,
	mchain_t ch,
	Tail &&... tail )
	{
		add_to_closer( to, index, std::move(ch) );
		add_to_closer( to, index + 1, std::forward< Tail >(tail)... );
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
template< typename... Tail >
mchain_auto_close_details::auto_closer_t< sizeof...(Tail) >
auto_close_mchains(
	//! Close mode for all mchains.
	//! This value will be passed to all close() calls.
	mchain_props::close_mode_t close_mode,
	//! Chains to be closed.
	Tail &&... tail )
	{
		using namespace mchain_auto_close_details;

		auto_closer_t< sizeof...(tail) > closer{ close_mode };
		add_to_closer( closer, 0, std::forward< Tail >(tail)... );

		return closer;
	}

/*!
 * \brief Helper function for automatic closing of mchains with
 * retaining their content.
 *
 * Usage example:
 * \code
	so_5::environment_t & env = ...;
	auto ch1 = create_mchain(env);
	auto ch2 = create_mchain(env);
	auto ch_closer = so_5::auto_close_retain_content(ch1, ch2);
	...
 * \endcode
 *
 * \note This is just a shorthand for:
 * \code
	so_5::auto_close_mchains(
		so_5::mchain_props::close_mode_t::retain_content, chains... );
 * \endcode
 *
 * \tparam Tail List of mchains.
 *
 * \since
 * v.5.5.16
 */
template< typename... Tail >
mchain_auto_close_details::auto_closer_t< sizeof...(Tail) >
auto_close_retain_content( Tail &&... tail )
	{
		return auto_close_mchains(
				mchain_props::close_mode_t::retain_content,
				std::forward< Tail >(tail)... );
	}

/*!
 * \brief Helper function for automatic closing of mchains with
 * dropping their content.
 *
 * Usage example:
 * \code
	so_5::environment_t & env = ...;
	auto ch1 = create_mchain(env);
	auto ch2 = create_mchain(env);
	auto ch_closer = so_5::auto_close_drop_content(ch1, ch2);
	...
 * \endcode
 *
 * \note This is just a shorthand for:
 * \code
	so_5::auto_close_mchains(
		so_5::mchain_props::close_mode_t::drop_content, chains... );
 * \endcode
 *
 * \tparam Tail List of mchains.
 *
 * \since
 * v.5.5.16
 */
template< typename... Tail >
mchain_auto_close_details::auto_closer_t< sizeof...(Tail) >
auto_close_drop_content( Tail &&... tail )
	{
		return auto_close_mchains(
				mchain_props::close_mode_t::drop_content,
				std::forward< Tail >(tail)... );
	}

//
// mchain_master_handle_t
//
/*!
 * \brief Helper class for automatic close of a mchain at
 * the destruction of master handle instance.
 *
 * This class is intended for cases like that:
	\code
	auto ch = so_5::create_mchain( env );
	auto ch_closer = so_5::auto_close_drop_content( ch );
	...
	\endcode
 * The example shown above can be replaced by:
 * \code
	so_5::mchain_master_handle_t ch_handle(
		so_5::create_mchain( env ),
		so_5::mchain_props::close_mode_t::drop_content );

	// Or
	auto ch_handle = so_5::mchain_master_handle_t::make(
		so_5::create_mchain( env ),
		so_5::mchain_props::close_mode_t::drop_content );

	// Or
	auto ch_handle = so_5::mchain_master_handle_t::with_drop_content(
		so_5::create_mchain( env ) );
 * \endcode
 *
 * \note This class is moveable but not copyable.
 *
 * \since
 * v.5.5.17
 */
class mchain_master_handle_t
	{
	public :
		mchain_master_handle_t( const mchain_master_handle_t & ) = delete;
		mchain_master_handle_t &
		operator=( const mchain_master_handle_t & ) = delete;

		//! Initializing constructor.
		mchain_master_handle_t(
			//! A mchain itself.
			mchain_t chain,
			//! Close mode for mchain.
			mchain_props::close_mode_t close_mode ) SO_5_NOEXCEPT
			:	m_chain( std::move(chain) )
			,	m_close_mode( close_mode )
			{}

		//! Move constructor.
		mchain_master_handle_t(
			mchain_master_handle_t && handle ) SO_5_NOEXCEPT
			:	m_chain( std::move(handle.m_chain) )
			,	m_close_mode( handle.m_close_mode )
			{}

		//! Destructor closes the chain.
		~mchain_master_handle_t()
			{
				if( m_chain )
					m_chain->close( m_close_mode );
			}

		//! Move operator.
		mchain_master_handle_t &
		operator=( mchain_master_handle_t && handle ) SO_5_NOEXCEPT
			{
				mchain_master_handle_t tmp( std::move(handle) );
				this->swap(tmp);
				return *this;
			}

		//! Swap operation.
		void
		swap( mchain_master_handle_t & other ) SO_5_NOEXCEPT
			{
				m_chain.swap( other.m_chain );
				std::swap( m_close_mode, other.m_close_mode );
			}

		//! Get the mchain.
		const mchain_t &
		get() const SO_5_NOEXCEPT { return m_chain; }

		//! Get the mchain.
		const mchain_t &
		operator*() const SO_5_NOEXCEPT { return m_chain; }

		//! \name Helpers methods for master handle creation.
		//! \{
		/*!
		 * \par Usage example:
		 * \code
			void demo( so_5::environment_t & env, app_config & config )
			{
				std::thread worker;
				auto worker_joiner = so_5::auto_join( worker );

				auto chain = so_5::mchain_master_handle_t::make(
					create_mchain( env ),
					detect_close_mode( config ) );

				worker = std::thread( [ch = *chain]{ ... } );
				...
			}
		 * \endcode
		 */
		inline static mchain_master_handle_t
		make(
			mchain_t chain,
			mchain_props::close_mode_t close_mode ) SO_5_NOEXCEPT
			{
				return mchain_master_handle_t( std::move(chain), close_mode );
			}

		/*!
		 * \par Usage example:
		 * \code
			void demo( so_5::environment_t & env )
			{
				std::thread worker;
				auto worker_joiner = so_5::auto_join( worker );

				auto chain = so_5::mchain_master_handle_t::with_drop_content(
					create_mchain( env ) );

				worker = std::thread( [ch = *chain]{ ... } );
				...
			}
		 * \endcode
		 */
		inline static mchain_master_handle_t
		with_drop_content( mchain_t chain ) SO_5_NOEXCEPT
			{
				return mchain_master_handle_t( std::move(chain),
						mchain_props::close_mode_t::drop_content );
			}

		/*!
		 * \par Usage example:
		 * \code
			void demo( so_5::environment_t & env )
			{
				std::thread worker;
				auto worker_joiner = so_5::auto_join( worker );

				auto chain = so_5::mchain_master_handle_t::with_retain_content(
					create_mchain( env ) );

				worker = std::thread( [ch = *chain]{ ... } );
				...
			}
		 * \endcode
		 */
		inline static mchain_master_handle_t
		with_retain_content( mchain_t chain ) SO_5_NOEXCEPT
			{
				return mchain_master_handle_t( std::move(chain),
						mchain_props::close_mode_t::retain_content );
			}
		//! \}
	private :
		//! Chain to be hold.
		mchain_t m_chain;
		//! Close mode for mchain.
		mchain_props::close_mode_t m_close_mode;
	};

} /* namespace so_5 */

