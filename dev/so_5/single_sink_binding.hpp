/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Stuff for single_sink_binding implementation.
 *
 * \since v.5.8.0
 */

#pragma once

#include <so_5/mbox.hpp>

#include <so_5/details/rollback_on_exception.hpp>

#include <optional>
#include <type_traits>

namespace so_5
{

//
// single_sink_binding_t
//
//FIXME: document this!
class single_sink_binding_t
	{
		struct binding_info_t
			{
				mbox_t m_source;
				std::type_index m_msg_type;
				msink_t m_sink_owner;
				// NOTE: may be nullptr!
				delivery_filter_unique_ptr_t m_delivery_filter;

				binding_info_t(
					const mbox_t & source,
					const std::type_index & msg_type,
					const msink_t & sink_owner,
					delivery_filter_unique_ptr_t delivery_filter ) noexcept
					:	m_source{ source }
					,	m_msg_type{ msg_type }
					,	m_sink_owner{ sink_owner }
					,	m_delivery_filter{ std::move(delivery_filter) }
					{}
			};

		std::optional< binding_info_t > m_info;

	public:
		friend void
		swap( single_sink_binding_t & a, single_sink_binding_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_info, b.m_info );
			}

		single_sink_binding_t() noexcept = default;

		~single_sink_binding_t() noexcept
			{
				clear();
			}

		single_sink_binding_t(
			const single_sink_binding_t & ) = delete;
		single_sink_binding_t &
		operator=(
			const single_sink_binding_t & ) = delete;

		single_sink_binding_t(
			single_sink_binding_t && other ) noexcept
			:	m_info{ std::exchange( other.m_info, std::nullopt ) }
			{}

		single_sink_binding_t &
		operator=(
			single_sink_binding_t && other ) noexcept
			{
				single_sink_binding_t tmp{ std::move(other) };
				swap( *this, tmp );
				return *this;
			}

		[[nodiscard]]
		bool
		has_value() const noexcept { return m_info.has_value(); }

		[[nodiscard]]
		bool
		empty() const noexcept { return !has_value(); }

		void
		clear() noexcept
			{
				if( m_info.has_value() )
					{
						if( m_info->m_delivery_filter )
							{
								m_info->m_source->drop_delivery_filter(
										m_info->m_msg_type,
										m_info->m_sink_owner->sink() );
								m_info->m_delivery_filter.reset();
							}

						m_info->m_source->unsubscribe_event_handlers(
								m_info->m_msg_type,
								m_info->m_sink_owner->sink() );

						m_info = std::nullopt;
					}
			}

		void
		unbind() noexcept
			{
				clear();
			}

		//FIXME: document this!
		void
		bind_for_msg_type(
			const std::type_index & msg_type,
			const mbox_t & source,
			const msink_t & sink_owner )
			{
				// Previous binding has to be dropped.
				clear();

				source->subscribe_event_handler(
						msg_type,
						sink_owner->sink() );

				// Don't expect that this action can throw!
				so_5::details::invoke_noexcept_code( [&] {
						m_info = binding_info_t{
								source, msg_type, sink_owner, delivery_filter_unique_ptr_t{}
							};
					} );
			}

		//FIXME: document this!
		template< typename Msg >
		void
		bind(
			const mbox_t & source,
			const msink_t & sink_owner )
			{
				this->bind_for_msg_type(
						message_payload_type<Msg>::subscription_type_index(),
						source,
						sink_owner );
			}

		//FIXME: document this!
		void
		bind_for_msg_type(
			const std::type_index & msg_type,
			const mbox_t & source,
			const msink_t & sink_owner,
			delivery_filter_unique_ptr_t delivery_filter )
			{
				//FIXME: should we check that `sink_owner` isn't null (the same for `source`)?
				//FIXME: there should be check that delivery_filter isn't nullptr.

				// Previous binding has to be dropped.
				clear();

				// Assume that this check was performed by caller.
				// ensure_not_signal< Msg >();

				source->set_delivery_filter(
						msg_type,
						*delivery_filter,
						sink_owner->sink() );

				so_5::details::do_with_rollback_on_exception(
						[&]() {
							source->subscribe_event_handler(
									msg_type,
									sink_owner->sink() );
						},
						[&]() {
							source->drop_delivery_filter( msg_type, sink_owner->sink() );
						} );

				// Don't expect that this action can throw!
				so_5::details::invoke_noexcept_code( [&] {
						m_info = binding_info_t{
								source, msg_type, sink_owner, std::move(delivery_filter)
							};
					} );
			}

		//FIXME: document this!
		template< typename Msg >
		void
		bind(
			const mbox_t & source,
			const msink_t & sink_owner,
			delivery_filter_unique_ptr_t delivery_filter )
			{
				ensure_not_signal< Msg >();

				bind_for_msg_type(
						message_payload_type< Msg >::subscription_type_index(),
						source,
						sink_owner,
						std::move(delivery_filter) );
			}

		//FIXME: document this!
		template< typename Msg, typename Lambda >
		void
		bind(
			const mbox_t & source,
			const msink_t & sink_owner,
			Lambda && filter )
			{
				using namespace so_5::details::lambda_traits;

				using lambda_type = std::remove_reference_t< Lambda >;
				using argument_type =
						typename argument_type_if_lambda< lambda_type >::type;

				//FIXME: can this check be written that way to show Msg and argument_type
				//in the error message?
				// For cases when Msg is mutable_msg<M>.
				static_assert(
						std::is_same_v<
								typename so_5::message_payload_type<Msg>::payload_type,
								argument_type >,
						"lambda expects a different message type" );

				delivery_filter_unique_ptr_t filter_holder{
						new low_level_api::lambda_as_filter_t< lambda_type, argument_type >(
								std::move(filter) )
					};

				this->bind< Msg >( source, sink_owner, std::move(filter_holder) );
			}
	};

} /* namespace so_5 */

