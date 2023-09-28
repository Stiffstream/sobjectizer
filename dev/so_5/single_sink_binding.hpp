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

namespace sink_bindings_details
{

/*!
 * @brief Helper to have more information in compiler output if static_assert
 * fails.
 *
 * If static_assert fails compiler will show actual names of Subscription_Type
 * and Delivery_Filter_Arg_Type in an error message.
 */
template<
	typename Subscription_Type,
	typename Delivery_Filter_Arg_Type>
void
ensure_valid_argument_for_delivery_filter()
	{
//FIXME: can we check that Delivery_Filter_Arg_Type is a value type or const reference?
		static_assert(
				std::is_same_v< Subscription_Type, Delivery_Filter_Arg_Type >,
				"delivery filter lambda expects a different message type" );
	}

} /* namespace sink_bindings_details */

//
// single_sink_binding_t
//
/*!
 * \brief Helper class for managing single sink bindings.
 *
 * An instance of single_sink_binding_t drops the binding in the destructor.
 * If it's necessary to drop the binding manually then clear()/unbind() methods
 * can be used.
 *
 * Usage examples:
 * \code
 * // Use as a part of an agent.
 * class coordinator final : public so_5::agent_t
 * {
 * 	const so_5::mbox_t broadcasting_mbox_;
 * 	so_5::single_sink_binding_t bindings_;
 * ...
 * 	void on_some_event(mhood_t<msg_some_command> cmd) {
 * 		// Create a child coop and bind an agent to broadcasting mbox.
 * 		so_5::introduce_child_coop(*this, [](so_5::coop_t & coop) {
 * 				auto * worker = coop.make_agent<worker>(...);
 * 				auto worker_msink = so_5::wrap_to_msink(worker->so_direct_mbox());
 *
 * 				bindings_.bind<msg_some_data>(broadcasting_mbox_, worker_msink);
 * 				...
 * 			});
 * 	}
 * };
 *
 * // Use as object controlled by a coop.
 * so_5::environment_t & env = ...;
 * env.introduce_coop([](so_5::coop_t & coop) {
 * 		const auto broadcasting_mbox = coop.environment().create_mbox();
 * 		auto * first = coop.make_agent<first_worker>(...);
 * 		auto * first_binding = coop.take_under_control(
 * 			std::make_unique<so_5::single_sink_binding_t>() );
 * 		first_binding->bind<msg_some_data>(broadcasting_mbox,
 * 			so_5::wrap_to_msink(first->so_direct_mbox()));
 *
 * 		auto * second = coop.make_agent<second_worker>(...);
 * 		auto * second_binding = coop.take_under_control(
 * 			std::make_unique<so_5::single_sink_binding_t>() );
 * 		second_binding->bind<msg_some_data>(broadcasting_mbox,
 * 			so_5::wrap_to_msink(second->so_direct_mbox()));
 * 		...
 * 	});
 * \endcode
 *
 * There is a principial difference between single_sink_binding_t and
 * multi_sink_binding_t: if bind() is called for single_sink_binding_t when
 * the binding is exists, then old binding will be dropped and new binding will
 * be created. For example, this is a valid behavior for single_sink_binding_t:
 * \code
 * const so_5::mbox_t & source = ...;
 * const so_5::msink_t & dest = ...;
 * binding.bind<my_message>(source, dest); // New binding created.
 * ...
 * binding.bind<my_message>(source, dest); // Old binding will be removed
 *    // and new binding will be created (despite the fact that source and dest
 *    // are the same).
 * \endcode
 * Contrary, multi_sink_binding_t::bind() throws if a binding for triplet
 * (message, source, dest) already exists.
 *
 * \attention
 * The instance of single_sink_binding_t is not thread safe. If a user wants
 * to work with an instance of single_sink_binding_t from different
 * threads then the user has to protect the instance by her/himself.
 *
 * \note
 * This class is Moveable, but is not Copyable.
 *
 * \since v.5.8.0
 */
class single_sink_binding_t
	{
		/*!
		 * Actual information about binding.
		 */
		struct binding_info_t
			{
				//! The source mbox.
				mbox_t m_source;
				//! Type of message/signal.
				std::type_index m_msg_type;
				//! The destination for messages/signals.
				msink_t m_sink_owner;
				//! Optional delivery filter.
				/*!
				 * May be nullptr. Can't be used for signals.
				 */
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

		/*!
		 * Information about the current binding.
		 *
		 * Empty value means that there is no binding at the moment.
		 */
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

		/*!
		 * \retval true if binding exists
		 * \retval false if there is no binding at the moment.
		 */
		[[nodiscard]]
		bool
		has_value() const noexcept { return m_info.has_value(); }

		/*!
		 * \retval false if binding exists
		 * \retval true if there is no binding at the moment.
		 */
		[[nodiscard]]
		bool
		empty() const noexcept { return !has_value(); }

		/*!
		 * Remove the current binding.
		 *
		 * It's safe to call this method even if there is no binding
		 * at the moment.
		 *
		 * The object can be used for creation of a new binding after calling
		 * clear() method. For example:
		 * \code
		 * so_5::single_sink_binding_t & binding = ...;
		 *
		 * binding.clear(); // Object is empty now.
		 *
		 * const so_5::mbox_t & source = ...;
		 * const so_5::msink_t & dest = ...;
		 * binding.bind<my_message>(source, dest); // New binding created.
		 * \endcode
		 */
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

						m_info->m_source->unsubscribe_event_handler(
								m_info->m_msg_type,
								m_info->m_sink_owner->sink() );
					}
				// NOTE: initially this call was inside if(),
				// but it led to problem with GCC-13.1 with -Werror,
				// -Wall, -Wpedantic. GCC-13.1 complained sometimes
				// (test/so_5/mbox/sink_binding/single_sink_too_deep/main.cpp)
				// about the use of uninitialized pointer somewhere inside
				// implementation of std::unique_ptr.
				// It seems to be a bug in the compiler. As a workaround
				// a call to `reset` is moved outside of if().
				m_info.reset();
			}

		/*!
		 * A synonym for the clear() method.
		 */
		void
		unbind() noexcept
			{
				clear();
			}

		/*!
		 * Helper method for creation of a new binding for case when
		 * the type of message/signal is represented as std::type_index.
		 *
		 * \note
		 * This method is intended to be used for internal use. It's not
		 * guaranteed that it won't be changed (or removed) in future
		 * versions of SObjectizer.
		 */
		void
		bind_for_msg_type(
			//! Type of the message/signal.
			const std::type_index & msg_type,
			//! The source mbox.
			const mbox_t & source,
			//! The destination for messages/signals.
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

		/*!
		 * Create a binding for message/signal of type \a Msg from mbox \a source
		 * to the destination \a sink_owner.
		 *
		 * This binding won't use a delivery filter.
		 *
		 * If the object already holds a binding the current binding will be
		 * removed before the creation of a new one.
		 *
		 * Usage example:
		 * \code
		 * const so_5::mbox_t & source = ...;
		 * const so_5::msink_t & dest = ...;
		 * auto binding = std::make_unique< so_5::single_sink_binding_t >();
		 *
		 * binding->bind<my_message>(source, dest);
		 * \endcode
		 *
		 * It it's required to make a binding for a mutable message then
		 * so_5::mutable_msg marker has to be used:
		 * \code
		 * const so_5::mbox_t & source = ...;
		 * const so_5::msink_t & dest = ...;
		 * auto binding = std::make_unique< so_5::single_sink_binding_t >();
		 *
		 * binding->bind< so_5::mutable_msg<my_message> >(source, dest);
		 * \endcode
		 */
		template< typename Msg >
		void
		bind(
			//! The source mbox.
			const mbox_t & source,
			//! The destination for messages/signals.
			const msink_t & sink_owner )
			{
				this->bind_for_msg_type(
						message_payload_type<Msg>::subscription_type_index(),
						source,
						sink_owner );
			}

		/*!
		 * Helper method for creation of a new binding for case when
		 * the type of message is represented as std::type_index.
		 *
		 * If the object already holds a binding the current binding will be
		 * removed before the creation of a new one.
		 *
		 * \note
		 * This method is intended to be used for internal use. It's not
		 * guaranteed that it won't be changed (or removed) in future
		 * versions of SObjectizer.
		 *
		 * \note
		 * This method can't be used for binding signals.
		 */
		void
		bind_for_msg_type(
			//! The type of the message.
			const std::type_index & msg_type,
			//! The source mbox.
			const mbox_t & source,
			//! The destination for messages.
			const msink_t & sink_owner,
			//! Delivery filter to be used. It shouldn't be nullptr.
			delivery_filter_unique_ptr_t delivery_filter )
			{
				so_5::low_level_api::ensure_not_null( delivery_filter );

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

		/*!
		 * Create a binding for message of type \a Msg from mbox \a source
		 * to the destination \a sink_owner.
		 *
		 * This binding should use delivery filter \a delivery_filter.
		 *
		 * If the object already holds a binding the current binding will be
		 * removed before the creation of a new one.
		 *
		 * \note
		 * This method can't be used for binding signals.
		 */
		template< typename Msg >
		void
		bind(
			//! The source mbox.
			const mbox_t & source,
			//! The destination for messages.
			const msink_t & sink_owner,
			//! Delivery filter to be used. It shouldn't be nullptr.
			delivery_filter_unique_ptr_t delivery_filter )
			{
				ensure_not_signal< Msg >();

				so_5::low_level_api::ensure_not_null( delivery_filter );

				bind_for_msg_type(
						message_payload_type< Msg >::subscription_type_index(),
						source,
						sink_owner,
						std::move(delivery_filter) );
			}

		/*!
		 * Create a binding for message of type \a Msg from mbox \a source
		 * to the destination \a sink_owner.
		 *
		 * The lambda (or functor) \a filter will be used as delivery filter
		 * for messages.
		 *
		 * If the object already holds a binding the current binding will be
		 * removed before the creation of a new one.
		 *
		 * \note
		 * This method can't be used for binding signals.
		 *
		 * Usage example:
		 * \code
		 * const so_5::mbox_t & source = ...;
		 * const so_5::msink_t & dest = ...;
		 * auto binding = std::make_unique< so_5::single_sink_binding_t >();
		 *
		 * binding->bind<my_message>(source, dest,
		 * 	[](const my_message & msg) {
		 * 		... // should return `true` or `false`.
		 * 	});
		 * \endcode
		 *
		 * It it's required to make a binding for a mutable message then
		 * so_5::mutable_msg marker has to be used, but note the type of
		 * delivery filter argument:
		 * \code
		 * const so_5::mbox_t & source = ...;
		 * const so_5::msink_t & dest = ...;
		 * auto binding = std::make_unique< so_5::single_sink_binding_t >();
		 *
		 * binding->bind< so_5::mutable_msg<my_message> >(source, dest,
		 * 	[](const my_message & msg) {
		 * 		... // should return `true` or `false`.
		 * 	});
		 * \endcode
		 */
		template< typename Msg, typename Lambda >
		void
		bind(
			//! The source mbox.
			const mbox_t & source,
			//! The destination for messages.
			const msink_t & sink_owner,
			//! Filter to be used.
			Lambda && filter )
			{
				using namespace so_5::details::lambda_traits;

				using lambda_type = std::remove_reference_t< Lambda >;
				using argument_type =
						typename argument_type_if_lambda< lambda_type >::type;

				// For cases when Msg is mutable_msg<M>.
				sink_bindings_details::ensure_valid_argument_for_delivery_filter<
						typename so_5::message_payload_type<Msg>::payload_type,
						argument_type
					>();

				delivery_filter_unique_ptr_t filter_holder{
						new low_level_api::lambda_as_filter_t< lambda_type, argument_type >(
								std::move(filter) )
					};

				this->bind< Msg >( source, sink_owner, std::move(filter_holder) );
			}
	};

} /* namespace so_5 */

