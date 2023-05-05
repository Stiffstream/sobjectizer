/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Helpers for working with a chain of coops for the final deregistration.
 * \since v.5.8.0
 */

#pragma once

#include <so_5/impl/coop_private_iface.hpp>
#include <so_5/impl/internal_env_iface.hpp>

#include <so_5/coop.hpp>

#include <utility>

namespace so_5::impl
{

//
// final_dereg_chain_holder_t
//
//FIXME: document this!
/*!
 * \note
 * This class isn't Copyable, not Moveable.
 *
 * \since v.5.8.0
 */
class final_dereg_chain_holder_t
	{
		/*!
		 * \brief Number of items in the chain of coops for the final
		 * deregistration.
		 *
		 * This value is necessary for stats.
		 *
		 * \since v.5.8.0
		 */
		std::size_t m_final_dereg_chain_size{};

		/*!
		 * \brief The head of the chain of coops for the final deregistration.
		 *
		 * It may be nullptr. It means that the chain is empty now.
		 *
		 * \since v.5.8.0
		 */
		coop_shptr_t m_final_dereg_chain_head;

		/*!
		 * \brief The tail of the chain of coops for the final deregistration.
		 *
		 * This value is used for fast addition of a new coop to the chain.
		 *
		 * It may be nullptr in the case when the chain is empty.
		 *
		 * \since v.5.8.0
		 */
		coop_shptr_t m_final_dereg_chain_tail;

	public:
		final_dereg_chain_holder_t() = default;
		//FIXME: should here be a check that the chain is empty?
		~final_dereg_chain_holder_t() = default;

		final_dereg_chain_holder_t(
			const final_dereg_chain_holder_t & ) = delete;
		final_dereg_chain_holder_t &
		operator=(
			const final_dereg_chain_holder_t & ) = delete;

		final_dereg_chain_holder_t(
			final_dereg_chain_holder_t && ) = delete;
		final_dereg_chain_holder_t &
		operator=(
			final_dereg_chain_holder_t && ) = delete;

		void
		append( coop_shptr_t coop ) noexcept
			{
				++m_final_dereg_chain_size;
				if( !m_final_dereg_chain_head )
					m_final_dereg_chain_head = coop;
				if( m_final_dereg_chain_tail )
					coop_private_iface_t::set_next_in_final_dereg_chain(
							*m_final_dereg_chain_tail,
							coop );
				m_final_dereg_chain_tail = std::move(coop);
			}

		[[nodiscard]]
		std::size_t
		size() const noexcept
			{
				return m_final_dereg_chain_size;
			}

		[[nodiscard]]
		bool
		empty() const noexcept
			{
				return !static_cast<bool>( m_final_dereg_chain_head );
			}

		[[nodiscard]]
		coop_shptr_t
		giveout_current_chain() noexcept
			{
				coop_shptr_t head = std::exchange(
						m_final_dereg_chain_head,
						coop_shptr_t{} );
				m_final_dereg_chain_tail = coop_shptr_t{};
				m_final_dereg_chain_size = 0u;

				return head;
			}
	};

//
// process_final_dereg_chain
//
//FIXME: document this!
/*!
 * \since v.5.8.0
 */
inline void
process_final_dereg_chain( coop_shptr_t head ) noexcept
	{
		// Do final_deregister_coop for every item in the chain
		// one by one.
		while( head )
		{
			coop_shptr_t next =
					coop_private_iface_t::giveout_next_in_final_dereg_chain(
							*head );
			auto & env = head->environment();
			internal_env_iface_t{ env }.final_deregister_coop(
					std::move(head) );

			head = std::move(next);
		}
	}

} /* namespace so_5::impl */

