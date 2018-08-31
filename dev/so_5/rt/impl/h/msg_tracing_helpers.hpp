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

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5 {

namespace impl {

namespace msg_tracing_helpers {

namespace details {

using namespace so_5::details::ios_helpers;

//
// actual_trace_data_t
//
/*!
 * \brief An actual implementation of trace data interface.
 *
 * \since
 * v.5.5.22
 */
class SO_5_TYPE actual_trace_data_t : public so_5::msg_tracing::trace_data_t
	{
	public :
		virtual optional<current_thread_id_t>
		tid() const SO_5_NOEXCEPT override;

		virtual optional<const agent_t *>
		agent() const SO_5_NOEXCEPT override;

		virtual optional<std::type_index>
		msg_type() const SO_5_NOEXCEPT override;

		virtual optional<so_5::msg_tracing::msg_source_t>
		msg_source() const SO_5_NOEXCEPT override;

		virtual optional<so_5::msg_tracing::message_or_signal_flag_t>
		message_or_signal() const SO_5_NOEXCEPT override;

		virtual optional<so_5::msg_tracing::message_instance_info_t>
		message_instance_info() const SO_5_NOEXCEPT override;

		virtual optional<so_5::msg_tracing::compound_action_description_t>
		compound_action() const SO_5_NOEXCEPT override;

		virtual optional<const so_5::impl::event_handler_data_t *>
		event_handler_data_ptr() const SO_5_NOEXCEPT override;

		void
		set_tid( current_thread_id_t tid ) SO_5_NOEXCEPT;

		void
		set_agent( const agent_t * agent ) SO_5_NOEXCEPT;

		void
		set_msg_type( const std::type_index & msg_type ) SO_5_NOEXCEPT;

		void
		set_msg_source( so_5::msg_tracing::msg_source_t info ) SO_5_NOEXCEPT;

		void
		set_message_or_signal(
			so_5::msg_tracing::message_or_signal_flag_t flag ) SO_5_NOEXCEPT;

		void
		set_message_instance_info(
			so_5::msg_tracing::message_instance_info_t info ) SO_5_NOEXCEPT;

		void
		set_compound_action(
			so_5::msg_tracing::compound_action_description_t desc ) SO_5_NOEXCEPT;

		void
		set_event_handler_data_ptr(
			const so_5::impl::event_handler_data_t * ptr ) SO_5_NOEXCEPT;

	private :
		optional<current_thread_id_t> m_tid;
		optional<const agent_t *> m_agent;
		optional<std::type_index> m_msg_type;
		optional<so_5::msg_tracing::msg_source_t> m_msg_source;
		optional<so_5::msg_tracing::message_or_signal_flag_t> m_message_or_signal;
		optional<so_5::msg_tracing::message_instance_info_t> m_message_instance_info;
		optional<so_5::msg_tracing::compound_action_description_t> m_compound_action;
		optional<const so_5::impl::event_handler_data_t *> m_event_handler_data_ptr;
	};

struct overlimit_deep
	{
		unsigned int m_deep;

		// Note: this constructor is necessary for compatibility with MSVC++2013.
		overlimit_deep( unsigned int deep ) : m_deep{ deep } {}
	};

struct mbox_identification
	{
		mbox_id_t m_id;
	};

struct mbox_as_msg_source
	{
		const abstract_message_box_t & m_mbox;
	};

struct mbox_as_msg_destination
	{
		const abstract_message_box_t & m_mbox;
	};

struct mchain_as_msg_source
	{
		const abstract_message_chain_t & m_mchain;
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

struct original_msg_type
	{
		const std::type_index & m_type;
	};

struct type_of_removed_msg
	{
		const std::type_index & m_type;
	};

struct type_of_transformed_msg
	{
		const std::type_index & m_type;
	};

inline void
make_trace_to_1( std::ostream & s, current_thread_id_t tid )
	{
		s << "[tid=" << tid << "]";
	}

inline void
fill_trace_data_1( actual_trace_data_t & d, current_thread_id_t tid )
	{
		d.set_tid( tid );
	}

inline void
make_trace_to_1( std::ostream & s, mbox_identification id )
	{
		s << "[mbox_id=" << id.m_id << "]";
	}

inline void
fill_trace_data_1( actual_trace_data_t & d, mbox_identification id )
	{
		d.set_msg_source(
				so_5::msg_tracing::msg_source_t{
						id.m_id,
						so_5::msg_tracing::msg_source_type_t::unknown } );
	}


inline void
make_trace_to_1( std::ostream & s, mchain_identification id )
	{
		s << "[mchain_id=" << id.m_id << "]";
	}

inline void
fill_trace_data_1( actual_trace_data_t & d, mchain_identification id )
	{
		d.set_msg_source(
				so_5::msg_tracing::msg_source_t{
						id.m_id,
						so_5::msg_tracing::msg_source_type_t::mchain } );
	}

inline void
make_trace_to_1(
	std::ostream & s,
	const mbox_as_msg_source mbox )
	{
		make_trace_to_1( s, mbox_identification{ mbox.m_mbox.id() } );
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & d,
	const mbox_as_msg_source & mbox )
	{
		d.set_msg_source(
				so_5::msg_tracing::msg_source_t{
						mbox.m_mbox.id(),
						so_5::msg_tracing::msg_source_type_t::mbox } );
	}

inline void
make_trace_to_1(
	std::ostream & s,
	const mbox_as_msg_destination mbox )
	{
		make_trace_to_1( s, mbox_identification{ mbox.m_mbox.id() } );
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & /*d*/,
	const mbox_as_msg_destination & /*mbox*/ )
	{
		// Just for compilation.
	}

inline void
make_trace_to_1( std::ostream & s, const abstract_message_chain_t & chain )
	{
		make_trace_to_1( s, mchain_identification{ chain.id() } );
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & d,
	const abstract_message_chain_t & chain )
	{
		fill_trace_data_1( d, mchain_identification{ chain.id() } );
	}

inline void
make_trace_to_1(
	std::ostream & s,
	const original_msg_type msg_type )
	{
		s << "[msg_type=" << msg_type.m_type.name() << "]";
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & d,
	const original_msg_type msg_type )
	{
		d.set_msg_type( msg_type.m_type );
	}

inline void
make_trace_to_1(
	std::ostream & s,
	const type_of_removed_msg msg_type )
	{
		s << "removed:[msg_type=" << msg_type.m_type.name() << "]";
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & /*d*/,
	const type_of_removed_msg /*msg_type*/ )
	{
		// Just for compilation.
	}

inline void
make_trace_to_1(
	std::ostream & s,
	const type_of_transformed_msg msg_type )
	{
		s << "[msg_type=" << msg_type.m_type.name() << "]";
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & /*d*/,
	const type_of_transformed_msg /*msg_type*/ )
	{
		// Just for compilation.
	}

inline void
make_trace_to_1( std::ostream & s, const agent_t * agent )
	{
		s << "[agent_ptr=" << pointer{agent} << "]";
	}

inline void
fill_trace_data_1( actual_trace_data_t & d, const agent_t * agent )
	{
		d.set_agent( agent );
	}

inline void
make_trace_to_1( std::ostream & s, const state_t * state )
	{
		s << "[state=" << state->query_name() << "]";
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & /*d*/,
	const state_t * /*state*/ )
	{
		// Just for compilation.
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
fill_trace_data_1(
	actual_trace_data_t & d,
	const event_handler_data_t * handler )
	{
		d.set_event_handler_data_ptr( handler );
	}

inline void
make_trace_to_1(
	std::ostream & s,
	const so_5::message_limit::control_block_t * limit )
	{
		s << "[limit_ptr=" << pointer{limit} << "]";
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & /*d*/,
	const so_5::message_limit::control_block_t * /*limit*/ )
	{
		// Just for compilation.
	}

inline std::tuple<const void *, const void *>
detect_message_pointers( const message_ref_t & message )
	{
		// The first pointer is a pointer to envelope.
		// The second pointer is a pointer to payload.
		using msg_pointers = std::tuple< const void *, const void * >;
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
	}

inline void
make_trace_to_1( std::ostream & s, const message_ref_t & message )
	{
		const void * envelope = nullptr;
		const void * payload = nullptr;

		std::tie(envelope,payload) = detect_message_pointers(message);

		if( envelope )
			s << "[envelope_ptr=" << pointer{envelope} << "]";
		if( payload )
			s << "[payload_ptr=" << pointer{payload} << "]";
		else
			s << "[signal]";
		if( message_mutability_t::mutable_message == message_mutability(message) )
			s << "[mutable]";
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & d,
	const message_ref_t & message )
	{
		const void * envelope = nullptr;
		const void * payload = nullptr;

		std::tie(envelope,payload) = detect_message_pointers(message);

		if( !envelope && !payload )
			{
				// This is a signal.
				d.set_message_or_signal(
						so_5::msg_tracing::message_or_signal_flag_t::signal );
			}
		else
			{
				// This is a message.
				d.set_message_or_signal(
						so_5::msg_tracing::message_or_signal_flag_t::message );

				d.set_message_instance_info(
						so_5::msg_tracing::message_instance_info_t{
								envelope,
								payload,
								message_mutability(message) } );
			}
	}

inline void
make_trace_to_1( std::ostream & s, const overlimit_deep limit )
	{
		s << "[overlimit_deep=" << limit.m_deep << "]";
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & /*d*/,
	const overlimit_deep /*limit*/ )
	{
		// Just for compilation.
	}

inline void
make_trace_to_1( std::ostream & s, const composed_action_name name )
	{
		s << " " << name.m_1 << "." << name.m_2 << " ";
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & d,
	const composed_action_name name )
	{
		d.set_compound_action(
				so_5::msg_tracing::compound_action_description_t{
						name.m_1, name.m_2 } );
	}

inline void
make_trace_to_1( std::ostream & s, const text_separator text )
	{
		s << " " << text.m_text << " ";
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & /*d*/,
	const text_separator /*text*/ )
	{
		// Just for compilation.
	}

inline void
make_trace_to_1( std::ostream & s, chain_size size )
	{
		s << "[chain_size=" << size.m_size << "]";
	}

inline void
fill_trace_data_1(
	actual_trace_data_t & /*d*/,
	chain_size /*size*/ )
	{
		// Just for compilation.
	}

inline void
make_trace_to( std::ostream & ) {}

inline void
fill_trace_data( actual_trace_data_t & ) {}

template< typename A, typename... Other >
void
make_trace_to( std::ostream & s, A && a, Other &&... other )
	{
		make_trace_to_1( s, std::forward< A >(a) );
		make_trace_to( s, std::forward< Other >(other)... );
	}

template< typename A, typename... Other >
void
fill_trace_data( actual_trace_data_t & d, A && a, Other &&... other )
	{
		fill_trace_data_1( d, std::forward< A >(a) );
		fill_trace_data( d, std::forward< Other >(other)... );
	}

template< typename... Args >
void
make_trace(
	so_5::msg_tracing::holder_t & msg_tracing_stuff,
	Args &&... args ) SO_5_NOEXCEPT
	{
#if !defined( SO_5_HAVE_NOEXCEPT )
		so_5::details::invoke_noexcept_code( [&] {
#endif
				const auto tid = query_current_thread_id();

				// Since v.5.5.22 we should check the presence of filter.
				// If filter is present then we should pass a trace via filter.
				auto filter = msg_tracing_stuff.take_filter();
				bool need_trace = true;
				if( filter )
					{
						actual_trace_data_t data;
						fill_trace_data( data, tid, std::forward<Args>(args)... );

						need_trace = filter->filter( data );
					}

				if( need_trace )
					{
						// Trace message should go to the tracer.
						std::ostringstream s;

						make_trace_to( s, tid, std::forward< Args >(args)... );

						msg_tracing_stuff.tracer().trace( s.str() );
					}
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

				// NOTE: this dummy method added in v.5.5.19.3.
				template< typename... Args >
				void
				make_trace(
					const char * /*action_name_suffix*/,
					Args &&... /*args*/ ) const
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
		so_5::msg_tracing::holder_t & m_tracer;

	public :
		tracing_enabled_base( so_5::msg_tracing::holder_t & tracer )
			:	m_tracer( tracer )
			{}

		so_5::msg_tracing::holder_t &
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
				so_5::msg_tracing::holder_t & m_tracer;
				const abstract_message_box_t & m_mbox;
				const char * m_op_name;
				const std::type_index & m_msg_type;
				const message_ref_t & m_message;
				const details::overlimit_deep m_overlimit_deep;

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

				// Note: since v.5.5.19.3 this is a public method.
				template< typename... Args >
				void
				make_trace(
					const char * action_name_suffix,
					Args &&... args ) const
					{
						details::make_trace(
								m_tracer,
								details::mbox_as_msg_source{ m_mbox },
								details::composed_action_name{
										m_op_name, action_name_suffix },
								details::original_msg_type{ m_msg_type },
								m_message,
								m_overlimit_deep,
								std::forward< Args >(args)... );
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
								details::mbox_as_msg_destination{ *target } );
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
								details::mbox_as_msg_destination{ *target },
								details::type_of_transformed_msg{ msg_type },
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
			internal_env_iface_t{ demand.m_receiver->so_environment() }.msg_tracing_stuff(),
			demand.m_receiver,
			details::composed_action_name{ context_marker, "find_handler" },
			details::mbox_identification{ demand.m_mbox_id },
			details::original_msg_type{ demand.m_msg_type },
			demand.m_message_ref,
			&(demand.m_receiver->so_current_state()),
			search_result );
	}

/*!
 * \brief Helper for tracing the result of search of deadletter handler.
 *
 * \since
 * v.5.5.21
 */
inline void
trace_deadletter_handler_search_result(
	const execution_demand_t & demand,
	const char * context_marker,
	const event_handler_data_t * search_result )
	{
		details::make_trace(
			internal_env_iface_t{ demand.m_receiver->so_environment() }.msg_tracing_stuff(),
			demand.m_receiver,
			details::composed_action_name{ context_marker, "deadletter_handler" },
			details::mbox_identification{ demand.m_mbox_id },
			details::original_msg_type{ demand.m_msg_type },
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
				env.msg_tracing_stuff(),
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
				env.msg_tracing_stuff(),
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

				template< typename Queue >
				void stored( const Queue & /*queue*/ ) {}

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
		so_5::msg_tracing::holder_t & m_tracer;

		static const char *
		message_or_svc_request(
			invocation_type_t invocation )
			{
				return invocation_type_t::event == invocation ?
						"message" : "service_request";
			}

	public :
		mchain_tracing_enabled_base( so_5::msg_tracing::holder_t & tracer )
			:	m_tracer( tracer )
			{}

		so_5::msg_tracing::holder_t &
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
						details::original_msg_type{ d.m_msg_type },
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
						details::original_msg_type{ d.m_msg_type },
						d.m_message_ref );
			}

		class deliver_op_tracer
			{
			private :
				so_5::msg_tracing::holder_t & m_tracer;
				const abstract_message_chain_t & m_chain;
				const char * m_op_name;
				const std::type_index & m_msg_type;
				const message_ref_t & m_message;

				template< typename... Args >
				void
				make_trace(
					const char * action_name_suffix,
					Args &&... args ) const
					{
						details::make_trace(
								m_tracer,
								m_chain,
								details::composed_action_name{
										m_op_name, action_name_suffix },
								details::original_msg_type{ m_msg_type },
								m_message,
								std::forward< Args >(args)... );
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

				template< typename Queue >
				void
				stored( const Queue & queue )
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
								details::type_of_removed_msg{ d.m_msg_type },
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

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

