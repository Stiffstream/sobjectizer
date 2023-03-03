/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Stuff for sink_binding_holder implementation.
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

namespace low_level_api
{

struct sink_binding_holder_builder_t;

} /* namespace low_level_api */

//
// sink_binding_holder_t
//
//FIXME: document this!
class sink_binding_holder_t
	{
		friend class low_level_api::sink_binding_holder_builder_t;

		struct binding_info_t
			{
				mbox_t m_source;
				std::type_index m_msg_type;
				std::reference_wrapper< message_sink_t > m_sink;
				// NOTE: may be nullptr!
				delivery_filter_unique_ptr_t m_delivery_filter;

				binding_info_t(
					mbox_t & source,
					const std::type_index & msg_type,
					message_sink_t & sink,
					delivery_filter_unique_ptr_t delivery_filter ) noexcept
					:	m_source{ source }
					,	m_msg_type{ msg_type }
					,	m_sink{ sink }
					,	m_delivery_filter{ std::move(delivery_filter) }
					{}
			};

		std::optional< binding_info_t > m_info;

		sink_binding_holder_t(
			mbox_t & source,
			const std::type_index & msg_type,
			message_sink_t & sink,
			delivery_filter_unique_ptr_t delivery_filter ) noexcept
			:	m_info{
					std::in_place,
					source,
					msg_type,
					sink,
					std::move(delivery_filter)
				}
			{}

	public:
		friend void
		swap( sink_binding_holder_t & a, sink_binding_holder_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_info, b.m_info );
			}

		~sink_binding_holder_t() noexcept
			{
				clear();
			}

		sink_binding_holder_t(
			const sink_binding_holder_t & ) = delete;
		sink_binding_holder_t &
		operator=(
			const sink_binding_holder_t & ) = delete;

		sink_binding_holder_t(
			sink_binding_holder_t && other ) noexcept
			:	m_info{ std::exchange( other.m_info, std::nullopt ) }
			{}
		sink_binding_holder_t &
		operator=(
			sink_binding_holder_t && other ) noexcept
			{
				sink_binding_holder_t tmp{ std::move(other) };
				swap( *this, tmp );
				return *this;
			}

		[[nodiscard]]
		bool
		has_value() const noexcept { return m_info.has_value(); }

		[[nodiscard]]
		bool
		empty() const noexcept { return not has_value(); }

		void
		clear() noexcept
			{
				if( m_info.has_value() )
					{
						if( m_info->m_delivery_filter )
							{
								m_info->m_source->drop_delivery_filter(
										m_info->m_msg_type,
										m_info->m_sink.get() );
								m_info->m_delivery_filter.reset();
							}

						m_info->m_source->unsubscribe_event_handlers(
								m_info->m_msg_type,
								m_info->m_sink.get() );

						m_info = std::nullopt;
					}
			}

		void
		unbind() noexcept
			{
				clear();
			}
	};

namespace low_level_api
{

//
// sink_binding_holder_builder_t
//
//FIXME: document this!
struct sink_binding_holder_builder_t
	{
		template< typename... Args >
		[[nodiscard]]
		static sink_binding_holder_t
		make( Args &&... args ) noexcept
			{
				return sink_binding_holder_t{ std::forward<Args>(args)... };
			}
	};

} /* namespace low_level_api */

//FIXME: document this!
template< typename Msg >
[[nodiscard]]
sink_binding_holder_t
bind_sink(
	mbox_t & source,
	message_sink_t & sink )
	{
		const auto msg_type = message_payload_type<Msg>::subscription_type_index();

		source->subscribe_event_handler(
				msg_type,
				sink );

		return low_level_api::sink_binding_holder_builder_t::make(
				source, msg_type, sink, delivery_filter_unique_ptr_t{} );
	}

//FIXME: document this!
template< typename Msg >
[[nodiscard]]
sink_binding_holder_t
bind_sink(
	mbox_t & source,
	message_sink_t & sink,
	delivery_filter_unique_ptr_t delivery_filter )
	{
		//FIXME: there should be check that delivery_filter isn't nullptr.

		const auto msg_type = message_payload_type<Msg>::subscription_type_index();

		ensure_not_signal< Msg >();

		source->set_delivery_filter(
				msg_type,
				*delivery_filter,
				sink );

		do_with_rollback_on_exception(
				[&]() {
					source->subscribe_event_handler(
							msg_type,
							sink );
				},
				[&]() {
					source->drop_delivery_filter( msg_type, sink );
				} );

		return low_level_api::sink_binding_holder_builder_t::make(
				source, msg_type, sink, std::move(delivery_filter) );
	}

//FIXME: document this!
template< typename Msg, typename Lambda >
[[nodiscard]]
sink_binding_holder_t
bind_sink(
	mbox_t & source,
	message_sink_t & sink,
	Lambda && filter )
	{
		using namespace so_5::details::lambda_traits;
		using namespace delivery_filter_templates;

		using lambda_type = std::remove_reference_t< Lambda >;
		using argument_type =
				typename argument_type_if_lambda< lambda_type >::type;

		//FIXME: can this check be written that way to show Msg and argument_type
		//in the error message?
		// For cases when Msg is mutable_msg<M>.
		static_assert(
				std::is_same_v<
						so_5::message_payload_type<Msg>::payload_type,
						argument_type >,
				"lambda expects a different message type" );

		delivery_filter_unique_ptr_t filter_holder{
				new lambda_as_filter_t< lambda_type, argument_type >(
						std::move(filter) )
			};

		return bind_sink< Msg >( source, sink, std::move(filter_holder) );
	}

} /* namespace so_5 */

