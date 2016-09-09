/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.9
 *
 * \file
 * \brief Various helpers for message delivery tracing stuff.
 */

#pragma once

#include <so_5/h/msg_tracing.hpp>

#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/mchain.hpp>
#include <so_5/rt/h/agent.hpp>

#include <so_5/rt/impl/h/internal_env_iface.hpp>
#include <so_5/rt/impl/h/internal_message_iface.hpp>
#include <so_5/rt/impl/h/message_limit_action_msg_tracer.hpp>

#include <so_5/details/h/invoke_noexcept_code.hpp>
#include <so_5/details/h/ios_helpers.hpp>

#include <sstream>
#include <tuple>

namespace so_5 {

namespace impl {

namespace msg_tracing_helpers {

namespace details {

using namespace so_5::details::ios_helpers;

struct overlimit_deep
	{
		unsigned int m_deep;

		// NOTE: this constructor is necessary for compatibility with MSVC++2013.
		overlimit_deep( unsigned int deep ) : m_deep{ deep } {}
	};

struct mbox_identification
	{
		mbox_id_t m_id;
	};

struct mchain_identification
	{
		mbox_id_t m_id;
	};

struct text_separator
	{
		const char * m_text;
	};

struct composed_action_name
	{
		const char * m_1;
		const char * m_2;
	};

struct chain_size
	{
		std::size_t m_size;
	};

inline void
make_trace_to_1( std::ostream & s, mbox_identification id )
	{
		s << "[mbox_id=" << id.m_id << "]";
	}

inline void
make_trace_to_1( std::ostream & s, mchain_identification id )
	{
		s << "[mchain_id=" << id.m_id << "]";
	}

inline void
make_trace_to_1( std::ostream & s, const abstract_message_box_t & mbox )
	{
		make_trace_to_1( s, mbox_identification{ mbox.id() } );
	}

inline void
make_trace_to_1( std::ostream & s, const abstract_message_chain_t & chain )
	{
		make_trace_to_1( s, mchain_identification{ chain.id() } );
	}

inline void
make_trace_to_1( std::ostream & s, const std::type_index & msg_type )
	{
		s << "[msg_type=" << msg_type.name() << "]";
	}

inline void
make_trace_to_1( std::ostream & s, const agent_t * agent )
	{
		s << "[agent_ptr=" << pointer{agent} << "]";
	}

inline void
make_trace_to_1( std::ostream & s, const state_t * state )
	{
		s << "[state=" << state->query_name() << "]";
	}

inline void
make_trace_to_1( std::ostream & s, const event_handler_data_t * handler )
	{
		s << "[evt_handler=";
		if( handler )
			s << pointer{handler};
		else
			s << "NONE";
		s << "]";
	}

inline void
make_trace_to_1(
	std::ostream & s,
	const so_5::message_limit::control_block_t * limit )
	{
		s << "[limit_ptr=" << pointer{limit} << "]";
	}

inline void
make_trace_to_1( std::ostream & s, const message_ref_t & message )
	{
		// The first pointer is a pointer to envelope.
		// The second pointer is a pointer to payload.
		using msg_pointers = std::tuple< const void *, const void * >;

		auto detect_pointers = [&message]() -> msg_pointers {
			if( const message_t * envelope = message.get() )
				{
					// We can try cases with service requests and user-type messages.
					const void * payload =
							internal_message_iface_t{ *envelope }.payload_ptr();

					if( payload != envelope )
						// There are an envelope and payload inside it.
						return msg_pointers{ envelope, payload };
					else
						// There is only payload.
						return msg_pointers{ nullptr, envelope };
				}
			else
				// It is a signal there is nothing.
				return msg_pointers{ nullptr, nullptr };
		};

		const void * envelope = nullptr;
		const void * payload = nullptr;

		std::tie(envelope,payload) = detect_pointers();

		if( envelope )
			s << "[envelope_ptr=" << pointer{envelope} << "]";
		if( payload )
			s << "[payload_ptr=" << pointer{payload} << "]";
		else
			s << "[signal]";
	}

inline void
make_trace_to_1( std::ostream & s, const overlimit_deep limit )
	{
		s << "[overlimit_deep=" << limit.m_deep << "]";
	}

inline void
make_trace_to_1( std::ostream & s, const composed_action_name name )
	{
		s << " " << name.m_1 << "." << name.m_2 << " ";
	}

inline void
make_trace_to_1( std::ostream & s, const text_separator text )
	{
		s << " " << text.m_text << " ";
	}

inline void
make_trace_to_1( std::ostream & s, chain_size size )
	{
		s << "[chain_size=" << size.m_size << "]";
	}

inline void
make_trace_to( std::ostream & ) {}

template< typename A, typename... OTHER >
void
make_trace_to( std::ostream & s, A && a, OTHER &&... other )
	{
		make_trace_to_1( s, std::forward< A >(a) );
		make_trace_to( s, std::forward< OTHER >(other)... );
	}

template< typename... ARGS >
void
make_trace(
	so_5::msg_tracing::tracer_t & tracer,
	ARGS &&... args ) SO_5_NOEXCEPT
	{
#if !defined( SO_5_HAVE_NOEXCEPT )
		so_5::details::invoke_noexcept_code( [&] {
#endif
				std::ostringstream s;

				s << "[tid=" << query_current_thread_id() << "]";

				make_trace_to( s, std::forward< ARGS >(args)... );

				tracer.trace( s.str() );
#if !defined( SO_5_HAVE_NOEXCEPT )
			} );
#endif
	}

} /* namespace details */

//
// tracing_disabled_base
//
/*!
 * \since
 * v.5.5.9
 *
 * \brief Base class for a mbox for the case when message delivery
 * tracing is disabled.
 */
struct tracing_disabled_base
	{
		class deliver_op_tracer
			{
			public :
				deliver_op_tracer(
					const tracing_disabled_base &,
					const abstract_message_box_t &,
					const char *,
					const std::type_index &,
					const message_ref_t &,
					const unsigned int )
					{}

				void
				no_subscribers() const {}

				void
				push_to_queue( const agent_t * ) const {}

				void
				message_rejected(
					const agent_t *,
					const delivery_possibility_t ) const {}

				const so_5::message_limit::impl::action_msg_tracer_t *
				overlimit_tracer() const { return nullptr; }
			};
	};

//
// tracing_enabled_base
//
/*!
 * \since
 * v.5.5.9
 *
 * \brief Base class for a mbox for the case when message delivery
 * tracing is enabled.
 */
class tracing_enabled_base
	{
	private :
		so_5::msg_tracing::tracer_t & m_tracer;

	public :
		tracing_enabled_base( so_5::msg_tracing::tracer_t & tracer )
			:	m_tracer( tracer )
			{}

		so_5::msg_tracing::tracer_t &
		tracer() const
			{
				return m_tracer;
			}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

		class deliver_op_tracer
			:	protected so_5::message_limit::impl::action_msg_tracer_t
			{
			private :
				so_5::msg_tracing::tracer_t & m_tracer;
				const abstract_message_box_t & m_mbox;
				const char * m_op_name;
				const std::type_index & m_msg_type;
				const message_ref_t & m_message;
				const details::overlimit_deep m_overlimit_deep;

				template< typename... ARGS >
				void
				make_trace(
					const char * action_name_suffix,
					ARGS &&... args ) const
					{
						details::make_trace(
								m_tracer,
								m_mbox,
								details::composed_action_name{
										m_op_name, action_name_suffix },
								m_msg_type,
								m_message,
								m_overlimit_deep,
								std::forward< ARGS >(args)... );
					}

			public :
				deliver_op_tracer(
					const tracing_enabled_base & tracing_base,
					const abstract_message_box_t & mbox,
					const char * op_name,
					const std::type_index & msg_type,
					const message_ref_t & message,
					const unsigned int overlimit_reaction_deep )
					:	m_tracer( tracing_base.tracer() )
					,	m_mbox( mbox )
					,	m_op_name( op_name )
					,	m_msg_type( msg_type )
					,	m_message( message )
					,	m_overlimit_deep( overlimit_reaction_deep )
					{
					}

				void
				no_subscribers() const
					{
						make_trace( "no_subscribers" );
					}

				void
				push_to_queue( const agent_t * subscriber ) const
					{
						make_trace( "push_to_queue", subscriber );
					}

				void
				message_rejected(
					const agent_t * subscriber,
					const delivery_possibility_t status ) const
					{
						if( delivery_possibility_t::disabled_by_delivery_filter
								== status )
							{
								make_trace( "message_rejected", subscriber );
							}
					}

				const so_5::message_limit::impl::action_msg_tracer_t *
				overlimit_tracer() const { return this; }

			protected :
				virtual void
				reaction_abort_app(
					const agent_t * subscriber ) const SO_5_NOEXCEPT override
					{
						make_trace( "overlimit.abort", subscriber );
					}

				virtual void
				reaction_drop_message(
					const agent_t * subscriber ) const SO_5_NOEXCEPT override
					{
						make_trace( "overlimit.drop", subscriber );
					}

				virtual void
				reaction_redirect_message(
					const agent_t * subscriber,
					const mbox_t & target ) const SO_5_NOEXCEPT override
					{
						make_trace(
								"overlimit.redirect",
								subscriber,
								details::text_separator{ "==>" },
								*target );
					}

				virtual void
				reaction_transform(
					const agent_t * subscriber,
					const mbox_t & target,
					const std::type_index & msg_type,
					const message_ref_t & transformed ) const SO_5_NOEXCEPT override
					{
						make_trace(
								"overlimit.transform",
								subscriber,
								details::text_separator{ "==>" },
								*target,
								msg_type,
								transformed );
					}
			};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

	};

/*!
 * \since
 * v.5.5.9
 *
 * \brief Helper for tracing the result of event handler search.
 */
inline void
trace_event_handler_search_result(
	const execution_demand_t & demand,
	const char * context_marker,
	const event_handler_data_t * search_result )
	{
		details::make_trace(
			internal_env_iface_t{ demand.m_receiver->so_environment() }.msg_tracer(),
			demand.m_receiver,
			details::composed_action_name{ context_marker, "find_handler" },
			details::mbox_identification{ demand.m_mbox_id },
			demand.m_msg_type,
			demand.m_message_ref,
			&(demand.m_receiver->so_current_state()),
			search_result );
	}

/*!
 * \since
 * v.5.5.15
 *
 * \brief Helper for tracing the fact of leaving a state.
 *
 * \note This helper checks status of msg tracing by itself. It means that
 * it is safe to call this function if msg tracing is disabled.
 */
inline void
safe_trace_state_leaving(
	const agent_t & state_owner,
	const state_t & state )
{
	internal_env_iface_t env{ state_owner.so_environment() };

	if( env.is_msg_tracing_enabled() )
		details::make_trace(
				env.msg_tracer(),
				&state_owner,
				details::composed_action_name{ "state", "leaving" },
				&state );
}

/*!
 * \since
 * v.5.5.15
 *
 * \brief Helper for tracing the fact of entering into a state.
 *
 * \note This helper checks status of msg tracing by itself. It means that
 * it is safe to call this function if msg tracing is disabled.
 */
inline void
safe_trace_state_entering(
	const agent_t & state_owner,
	const state_t & state )
{
	internal_env_iface_t env{ state_owner.so_environment() };

	if( env.is_msg_tracing_enabled() )
		details::make_trace(
				env.msg_tracer(),
				&state_owner,
				details::composed_action_name{ "state", "entering" },
				&state );
}

//
// mchain_tracing_disabled_base
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Base class for a mchain for the case when message delivery
 * tracing is disabled.
 */
struct mchain_tracing_disabled_base
	{
		void trace_extracted_demand(
			const abstract_message_chain_t &,
			const mchain_props::demand_t & ) {}

		void trace_demand_drop_on_close(
			const abstract_message_chain_t &,
			const mchain_props::demand_t & ) {}

		class deliver_op_tracer
			{
			public :
				deliver_op_tracer(
					const mchain_tracing_disabled_base &,
					const abstract_message_chain_t &,
					const std::type_index &,
					const message_ref_t &,
					const invocation_type_t )
					{}

				template< typename QUEUE >
				void stored( const QUEUE & /*queue*/ ) {}

				void overflow_drop_newest() {}

				void overflow_remove_oldest( const so_5::mchain_props::demand_t & ) {}

				void overflow_throw_exception() {}

				void overflow_abort_app() {}
			};
	};

//
// mchain_tracing_enabled_base
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Base class for a mchain for the case when message delivery
 * tracing is enabled.
 */
class mchain_tracing_enabled_base
	{
	private :
		so_5::msg_tracing::tracer_t & m_tracer;

		static const char *
		message_or_svc_request(
			invocation_type_t invocation )
			{
				return invocation_type_t::event == invocation ?
						"message" : "service_request";
			}

	public :
		mchain_tracing_enabled_base( so_5::msg_tracing::tracer_t & tracer )
			:	m_tracer( tracer )
			{}

		so_5::msg_tracing::tracer_t &
		tracer() const
			{
				return m_tracer;
			}

		void
		trace_extracted_demand(
			const abstract_message_chain_t & chain,
			const mchain_props::demand_t & d )
			{
				details::make_trace(
						m_tracer,
						chain,
						details::composed_action_name{
								message_or_svc_request( d.m_demand_type ),
								"extracted" },
						d.m_msg_type,
						d.m_message_ref );
			}

		void
		trace_demand_drop_on_close(
			const abstract_message_chain_t & chain,
			const mchain_props::demand_t & d )
			{
				details::make_trace(
						m_tracer,
						chain,
						details::composed_action_name{
								message_or_svc_request( d.m_demand_type ),
								"dropped_on_close" },
						d.m_msg_type,
						d.m_message_ref );
			}

		class deliver_op_tracer
			{
			private :
				so_5::msg_tracing::tracer_t & m_tracer;
				const abstract_message_chain_t & m_chain;
				const char * m_op_name;
				const std::type_index & m_msg_type;
				const message_ref_t & m_message;

				template< typename... ARGS >
				void
				make_trace(
					const char * action_name_suffix,
					ARGS &&... args ) const
					{
						details::make_trace(
								m_tracer,
								m_chain,
								details::composed_action_name{
										m_op_name, action_name_suffix },
								m_msg_type,
								m_message,
								std::forward< ARGS >(args)... );
					}

			public :
				deliver_op_tracer(
					const mchain_tracing_enabled_base & tracing_base,
					const abstract_message_chain_t & chain,
					const std::type_index & msg_type,
					const message_ref_t & message,
					const invocation_type_t invocation )
					:	m_tracer( tracing_base.tracer() )
					,	m_chain( chain )
					,	m_op_name( message_or_svc_request( invocation ) )
					,	m_msg_type( msg_type )
					,	m_message( message )
					{}

				template< typename QUEUE >
				void
				stored( const QUEUE & queue )
					{
						make_trace( "stored", details::chain_size{ queue.size() } );
					}

				void
				overflow_drop_newest()
					{
						make_trace( "overflow.drop_newest" );
					}

				void
				overflow_remove_oldest( const so_5::mchain_props::demand_t & d )
					{
						make_trace( "overflow.remove_oldest",
								details::text_separator{ "removed:" },
								d.m_msg_type,
								d.m_message_ref );
					}

				void
				overflow_throw_exception()
					{
						make_trace( "overflow.throw_exception" );
					}

				void
				overflow_abort_app()
					{
						make_trace( "overflow.abort_app" );
					}
			};
	};

} /* namespace msg_tracing_helpers */

} /* namespace impl */

} /* namespace so_5 */

