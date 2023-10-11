/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Helpers to simplify usage of transform_then_redirect_sink.
 *
 * \since v.5.8.1
 */

#pragma once

#include <so_5/msinks/transform_then_redirect.hpp>

#include <so_5/single_sink_binding.hpp>
#include <so_5/multi_sink_binding.hpp>

namespace so_5
{

namespace bind_then_redirect_impl
{

} /* namespace bind_then_redirect_impl */

/*!
 * \brief Helper function to add transform_then_redirect msink to a binding
 * object.
 *
 * Type of source message is automatically deduced from the type of
 * \a transformer argument.
 *
 * Usage example:
 * \code
 * struct part { ... };
 * struct compound {
 * 	part m_first;
 * 	part m_second;
 * };
 *
 * ...
 * so_5::mbox_t src_mbox = ...;
 * so_5::mbox_t dest_mbox = ...;
 *
 * so_5::single_sink_binding_t binding;
 * so_5::bind_transformer(binding, src_mbox,
 * 	[dest_mbox](const compound & msg) {
 * 		return so_5::make_transformed<part>(
 * 			dest_mbox, // Destination for the transformed message.
 * 			msg.m_first // Initializer for the new `part` instance.
 * 		);
 * 	});
 * \endcode
 *
 * The \a transformer is expected to return a so_5::transformed_message_t
 * or std::optional<so_5::transformed_message_t>:
 * \code
 * so_5::bind_transformer(binding, src_mbox,
 * 	[dest_mbox](const compound & msg)
 * 		-> std::optional<so_5::transformed_message_t<part>>
 * 	{
 * 		if(should_message_be_transformed(msg)) {
 * 			return {
 * 				so_5::make_transformed<part>(
 * 					dest_mbox, // Destination for the transformed message.
 * 					msg.m_first // Initializer for the new `part` instance.
 * 			) };
 * 		}
 * 		else
 * 			return std::nullopt; // No transformation.
 * 	});
 * \endcode
 *
 * \note
 * Because the type of the source message is deduced from \a transformer
 * argument this helper can't be used for transforming mutable messages
 * and signals.
 *
 * \attention
 * The \a transformer can't have an argument in the form of `const auto &`:
 * \code
 * // Compilation error is expected here because the type of
 * // the transformer argument can't be deduced.
 * so_5::bind_transformer(binding, src, [](const auto & msg) {...});
 * \endcode
 *
 * \tparam Binding type of binding object. It's expected to be
 * so_5::single_sink_binding_t, so_5::multi_sink_binding_t or a similar
 * type with the same interface.
 *
 * \tparam Transformer type of transformer functor (a lambda or free function).
 *
 * \since v.5.8.1
 */
template< typename Binding, typename Transformer >
void
bind_transformer(
	//! Binding object to be used.
	Binding & binding,
	//! Source mbox. It must not be nullptr.
	const so_5::mbox_t & src_mbox,
	//! Transformer that produced so_5::transformed_message_t instance or
	//! std::optional<so_5::transformed_message_t> instance.
	Transformer && transformer )
	{
		using transformer_t = std::decay_t< Transformer >;
		using lambda_traits_t =
				so_5::details::lambda_traits::traits< transformer_t >;
		using transformer_arg_t =
				std::remove_cv_t< typename lambda_traits_t::argument_type >;

		binding.template bind< transformer_arg_t >(
				src_mbox,
				so_5::msinks::transform_then_redirect(
						src_mbox->environment(),
						std::forward<Transformer>(transformer) ) );
	}

/*!
 * \brief Helper function to add transform_then_redirect msink to a binding
 * object (with a delivery filter).
 *
 * Type of source message is automatically deduced from the type of
 * \a transformer argument.
 *
 * The \a filter is called first and only if it return `true` the
 * \a transformed is called for message transformation.
 *
 * Usage example:
 * \code
 * struct part { ... };
 * struct compound {
 * 	part m_first;
 * 	part m_second;
 * };
 *
 * ...
 * so_5::mbox_t src_mbox = ...;
 * so_5::mbox_t dest_mbox = ...;
 *
 * so_5::single_sink_binding_t binding;
 * so_5::bind_transformer(binding, src_mbox,
 * 	[dest_mbox](const compound & msg) {
 * 		return so_5::make_transformed<part>(
 * 			dest_mbox, // Destination for the transformed message.
 * 			msg.m_first // Initializer for the new `part` instance.
 * 		);
 * 	},
 * 	[](const compound & msg) { // or [](const auto & msg) {...}
 * 		return ...; // Some predicate.
 * 	});
 * \endcode
 *
 * The \a transformer is expected to return a so_5::transformed_message_t
 * or std::optional<so_5::transformed_message_t>.
 *
 * \note
 * Because the type of the source message is deduced from \a transformer
 * argument this helper can't be used for transforming mutable messages
 * and signals.
 *
 * \attention
 * The \a transformer can't have an argument in the form of `const auto &`:
 * \code
 * // Compilation error is expected here because the type of
 * // the transformer argument can't be deduced.
 * so_5::bind_transformer(binding, src,
 * 	// This would lead to compilation error.
 * 	[](const auto & msg) {...},
 * 	// `const auto &` can be used for a delivery filter.
 * 	[](const auto & msg) {...});
 * \endcode
 *
 * \tparam Binding type of binding object. It's expected to be
 * so_5::single_sink_binding_t, so_5::multi_sink_binding_t or a similar
 * type with the same interface.
 *
 * \tparam Transformer type of transformer functor (a lambda or free function).
 *
 * \tparam Delivery_Filter type of delivery filter functor (a lambda or free
 * function).
 *
 * \since v.5.8.1
 */
template< typename Binding, typename Transformer, typename Delivery_Filter >
void
bind_transformer(
	//! Binding object to be used.
	Binding & binding,
	//! Source mbox. It must not be nullptr.
	const so_5::mbox_t & src_mbox,
	//! Transformer that produced so_5::transformed_message_t instance or
	//! std::optional<so_5::transformed_message_t> instance.
	Transformer && transformer,
	//! Delivery filter for the source message.
	Delivery_Filter && filter )
	{
		using transformer_t = std::decay_t< Transformer >;
		using lambda_traits_t =
				so_5::details::lambda_traits::traits< transformer_t >;
		using transformer_arg_t =
				std::remove_cv_t< typename lambda_traits_t::argument_type >;

		binding.template bind< transformer_arg_t >(
				src_mbox,
				so_5::msinks::transform_then_redirect(
						src_mbox->environment(),
						std::forward<Transformer>(transformer) ),
				std::forward<Delivery_Filter>(filter) );
	}

/*!
 * \brief Helper function to add transform_then_redirect msink to a binding
 * object.
 *
 * The type of the source message is specified explicitly.
 *
 * \attention
 * Because the source message is specified explicitly, this helper
 * can be used for immutable, mutable messages, and signals.
 *
 * Usage example:
 * \code
 * struct part { ... };
 * struct compound {
 * 	part m_first;
 * 	part m_second;
 * };
 *
 * ...
 * so_5::mbox_t src_mbox = ...;
 * so_5::mbox_t dest_mbox = ...;
 *
 * so_5::single_sink_binding_t binding;
 * so_5::bind_transformer< so_5::mutable_msg<compound> >(binding, src_mbox,
 * 	[dest_mbox](compound & msg) { // or [dest_mbox](auto & msg)
 * 		return so_5::make_transformed<part>(
 * 			dest_mbox, // Destination for the transformed message.
 * 			std::move(msg.m_first) // Initializer for the new `part` instance.
 * 		);
 * 	});
 * \endcode
 *
 * If this helper is used for signals then \a transformer should be a
 * function without arguments:
 * \code
 * struct first_signal final : public so_5::signal_t {};
 * struct second_signal final : public so_5::signal_t {};
 *
 * ...
 * so_5::mbox_t src_mbox = ...;
 * so_5::mbox_t dest_mbox = ...;
 *
 * so_5::single_sink_binding_t binding;
 * so_5::bind_transformer< first_signal >(binding, src_mbox,
 * 	[dest_mbox]() {
 * 		return so_5::make_transformed<second_signal>(
 * 			dest_mbox // Destination for the transformed message.
 * 		);
 * 	});
 * \endcode
 *
 * The \a transformer is expected to return a so_5::transformed_message_t
 * or std::optional<so_5::transformed_message_t>:
 * \code
 * // so_5::immutable_msg is supported as well as so_5::mutable_msg.
 * so_5::bind_transformer< so_5::immutable_msg<compound> >(binding, src_mbox,
 * 	[dest_mbox](const auto & msg)
 * 		-> std::optional<so_5::transformed_message_t<part>>
 * 	{
 * 		if(should_message_be_transformed(msg)) {
 * 			return {
 * 				so_5::make_transformed<part>(
 * 					dest_mbox, // Destination for the transformed message.
 * 					msg.m_first // Initializer for the new `part` instance.
 * 			) };
 * 		}
 * 		else
 * 			return std::nullopt; // No transformation.
 * 	});
 * \endcode
 *
 * \note
 * The \a transformer can have an argument in the form of `const auto &`:
 * \code
 * so_5::bind_transformer< compound >(binding, src, [](const auto & msg) {...});
 * \endcode
 *
 * \tparam Expected_Msg type of the source message or signal. If `Msg` is the
 * message type, then `Msg`, `so_5::immutable_msg<Msg>` or
 * `so_5::mutable_msg<Msg>` can be used. If `Msg` is the signal type then
 * `Msg` or `so_5::immutable_msg<Msg>` is allowed.
 *
 * \tparam Binding type of binding object. It's expected to be
 * so_5::single_sink_binding_t, so_5::multi_sink_binding_t or a similar
 * type with the same interface.
 *
 * \tparam Transformer type of transformer functor (a lambda or free function).
 *
 * \since v.5.8.1
 */
template< typename Expected_Msg, typename Binding, typename Transformer >
void
bind_transformer(
	//! Binding object to be used.
	Binding & binding,
	//! Source mbox. It must not be nullptr.
	const so_5::mbox_t & src_mbox,
	//! Transformer that produced so_5::transformed_message_t instance or
	//! std::optional<so_5::transformed_message_t> instance.
	Transformer && transformer )
	{
		binding.template bind< Expected_Msg >(
				src_mbox,
				so_5::msinks::transform_then_redirect< Expected_Msg >(
						src_mbox->environment(),
						std::forward<Transformer>(transformer) ) );
	}

/*!
 * \brief Helper function to add transform_then_redirect msink to a binding
 * object (with a delivery filter).
 *
 * The type of the source message is specified explicitly.
 *
 * \attention
 * Because the source message is specified explicitly, this helper
 * can be used for immutable and mutable messages.
 *
 * \attention
 * This helper can't be used for signals because delivery filters are
 * not applicable to signals.
 *
 * Usage example:
 * \code
 * struct part { ... };
 * struct compound {
 * 	part m_first;
 * 	part m_second;
 * };
 *
 * ...
 * so_5::mbox_t src_mbox = ...;
 * so_5::mbox_t dest_mbox = ...;
 *
 * so_5::single_sink_binding_t binding;
 * so_5::bind_transformer< so_5::mutable_msg<compound> >(binding, src_mbox,
 * 	[dest_mbox](compound & msg) { // or [dest_mbox](auto & msg)
 * 		return so_5::make_transformed<part>(
 * 			dest_mbox, // Destination for the transformed message.
 * 			std::move(msg.m_first) // Initializer for the new `part` instance.
 * 		);
 * 	},
 * 	// Note the use of const, delivery filter always receives a const
 * 	// reference for a message to be filtered.
 * 	[](const compound & msg) { // or [](const auto & msg)
 * 		return ...; // Some predicate.
 * 	});
 * \endcode
 *
 * The \a transformer is expected to return a so_5::transformed_message_t
 * or std::optional<so_5::transformed_message_t>:
 * \code
 * so_5::bind_transformer< so_5::mutable_msg<compound> >(binding, src_mbox,
 * 	[dest_mbox](auto & msg)
 * 		-> std::optional<so_5::transformed_message_t<part>>
 * 	{
 * 		if(should_message_be_transformed(msg)) {
 * 			return {
 * 				so_5::make_transformed<part>(
 * 					dest_mbox, // Destination for the transformed message.
 * 					std::move(msg.m_first) // Initializer for the new `part` instance.
 * 			) };
 * 		}
 * 		else
 * 			return std::nullopt; // No transformation.
 * 	},
 * 	// Note the use of const, delivery filter always receives a const
 * 	// reference for a message to be filtered.
 * 	[](const compound & msg) { // or [](const auto & msg)
 * 		return ...; // Some predicate.
 * 	});
 * \endcode
 *
 * \note
 * The \a transformer can have an argument in the form of `const auto &`:
 * \code
 * so_5::bind_transformer< compound >(binding, src,
 * 	// Transformer.
 * 	[](const auto & msg) {...},
 * 	// Delivery filter
 * 	[](const auto & msg) {...});
 * \endcode
 *
 * \tparam Expected_Msg type of the source message or signal. When `Msg` is the
 * expected message type, then `Msg`, `so_5::immutable_msg<Msg>` or
 * `so_5::mutable_msg<Msg>` can be used.
 *
 * \tparam Binding type of binding object. It's expected to be
 * so_5::single_sink_binding_t, so_5::multi_sink_binding_t or a similar
 * type with the same interface.
 *
 * \tparam Transformer type of transformer functor (a lambda or free function).
 *
 * \since v.5.8.1
 */
template<
	typename Expected_Msg,
	typename Binding,
	typename Transformer,
	typename Delivery_Filter >
void
bind_transformer(
	//! Binding object to be used.
	Binding & binding,
	//! Source mbox. It must not be nullptr.
	const so_5::mbox_t & src_mbox,
	//! Transformer that produced so_5::transformed_message_t instance or
	//! std::optional<so_5::transformed_message_t> instance.
	Transformer && transformer,
	//! Delivery filter for the source message.
	Delivery_Filter && delivery_filter )
	{
		binding.template bind< Expected_Msg >(
				src_mbox,
				so_5::msinks::transform_then_redirect< Expected_Msg >(
						src_mbox->environment(),
						std::forward<Transformer>(transformer) ),
				std::forward<Delivery_Filter>(delivery_filter) );
	}

} /* namespace so_5 */

