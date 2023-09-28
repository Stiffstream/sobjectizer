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

//FIXME: document this!
template< typename Binding, typename Transformer >
void
bind_then_transform(
	Binding & binding,
	const so_5::mbox_t & src_mbox,
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

//FIXME: document this!
template< typename Expected_Msg, typename Binding, typename Transformer >
void
bind_then_transform(
	Binding & binding,
	const so_5::mbox_t & src_mbox,
	Transformer && transformer )
	{
		binding.template bind< Expected_Msg >(
				src_mbox,
				so_5::msinks::transform_then_redirect< Expected_Msg >(
						src_mbox->environment(),
						std::forward<Transformer>(transformer) ) );
	}

} /* namespace so_5 */

