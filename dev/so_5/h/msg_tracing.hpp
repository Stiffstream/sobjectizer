/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.9
 *
 * \file
 * \brief Stuff related to message delivery tracing.
 */

#pragma once

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>
#include <so_5/h/atomic_refcounted.hpp>
#include <so_5/h/current_thread_id.hpp>
#include <so_5/h/optional.hpp>
#include <so_5/h/types.hpp>

#include <string>
#include <memory>
#include <typeindex>

namespace so_5 {

class agent_t;

namespace impl {

struct event_handler_data_t;

} /* namespace impl */

namespace msg_tracing {

/*!
 * \since
 * v.5.5.9
 *
 * \brief Status of message delivery tracing.
 */
enum class status_t
	{
		//! Message delivery tracing is disabled.
		disabled,
		//! Message delivery tracing is enabled.
		enabled
	};

//
// tracer_t
//

/*!
 * \since
 * v.5.5.9
 *
 * \brief Interface of tracer object.
 */
class SO_5_TYPE tracer_t
	{
		// Note: clang-3.9 requires this on Windows platform.
		tracer_t( const tracer_t & ) = delete;
		tracer_t( tracer_t && ) = delete;
		tracer_t & operator=( const tracer_t & ) = delete;
		tracer_t & operator=( tracer_t && ) = delete;
	public :
		tracer_t() = default;
		virtual ~tracer_t() SO_5_NOEXCEPT = default;

		//! Store a description of message delivery action to the
		//! appropriate storage/stream.
		virtual void
		trace( const std::string & what ) SO_5_NOEXCEPT = 0;
	};

//
// tracer_unique_ptr_t
//

/*!
 * \since
 * v.5.5.9
 *
 * \brief A short alias for unique_ptr to tracer.
 */
using tracer_unique_ptr_t = std::unique_ptr< tracer_t >;

//
// Standard stream tracers.
//

/*!
 * \since
 * v.5.5.9
 *
 * \brief Factory for tracer which uses std::cout stream.
 */
SO_5_FUNC tracer_unique_ptr_t
std_cout_tracer();

/*!
 * \since
 * v.5.5.9
 *
 * \brief Factory for tracer which uses std::cerr stream.
 */
SO_5_FUNC tracer_unique_ptr_t
std_cerr_tracer();

/*!
 * \since
 * v.5.5.9
 *
 * \brief Factory for tracer which uses std::clog stream.
 */
SO_5_FUNC tracer_unique_ptr_t
std_clog_tracer();

/*!
 * \brief A flag for message/signal dichotomy.
 *
 * \since
 * v.5.5.22
 */
enum class message_or_signal_flag_t
	{
		message,
		signal
	};

/*!
 * \brief Type of message source.
 *
 * Message can be sent to a mbox or to a mchain. This mbox/mchain
 * will be a message source for a subscriber. Sometimes it is necessary
 * to know what is actual type of message source: mbox or mchain.
 *
 * \since
 * v.5.5.22
 */
enum class msg_source_type_t
	{
		mbox,
		mchain,
		//! There is no information about a type of actual message source.
		//! This information can be lost if message was redirected or
		//! transformed during delivery.
		//! Or there is no information about message source in the point where
		//! message is handled. For example, when message arrived to a subscriber
		//! it is unknown was message sent to mchain or to mbox.
		unknown
	};

/*!
 * \brief An information about message source.
 *
 * \since
 * v.5.5.22
 */
struct msg_source_t
	{
		//! ID of mbox or mchain.
		mbox_id_t m_id;
		//! Type of message source.
		msg_source_type_t m_type;
	};

/*!
 * \brief An information about a message instance.
 *
 * \since
 * v.5.5.22
 */
struct message_instance_info_t
	{
		//! A pointer to envelope.
		/*!
		 * Can be null if message is not enveloped into a special wrapper.
		 */
		const void * m_envelope;
		//! A pointer to payload.
		/*!
		 * Can't be null regardless of presence of envelope.
		 */
		const void * m_payload;
		//! Information about message mutability.
		message_mutability_t m_mutability;
	};

/*!
 * \brief An information about compound description of message-related action.
 *
 * \since
 * v.5.5.22
 */
struct compound_action_description_t
	{
		//! The first part of the description.
		/*!
		 * Can't be null.
		 */
		const char * m_first;
		//! The second part of the description.
		/*!
		 * Can't be null.
		 */
		const char * m_second;
	};

//
// trace_data_t
//
/*!
 * \brief An interface of object for accessing trace details.
 *
 * A reference to an object with this interface will be passed to
 * trace filter. This interface provides access to details of a trace message.
 *
 * Please note that not all data can be available for every trace message.
 * Because of that all methods returns optional values. It means that
 * the value returned must be checked for presense first. For example:
 * \code
 * const auto thr_id = so_5::query_current_thread_id();
 * so_5::msg_tracing::make_filter(
 * 	[thr_id](const so_5::msg_tracing::trace_data_t & td) {
 * 		// Try to get ID of thread.
 * 		const auto tid = td.tid();
 * 		// Check the presence of value first.
 * 		if(tid) {
 * 			// And only then we can check the value itself.
 * 			return thr_id == *tid;
 * 		}
 * 		else
 * 			return false;
 * 	} );
 * \endcode
 *
 * \note
 * Version 5.5.22 provides and experimental implementation of
 * trace filters. Because of that the content of this interface can be
 * a subject of change in the future versions of SObjectizer.
 *
 * \since
 * v.5.5.22
 */
class SO_5_TYPE trace_data_t
	{
	protected :
		trace_data_t(const trace_data_t &) = delete;
		trace_data_t& operator=(const trace_data_t &) = delete;

		trace_data_t() = default;
		virtual ~trace_data_t() SO_5_NOEXCEPT = default;

	public :
		//! Get the Thread ID from trace message.
		virtual optional<current_thread_id_t>
		tid() const SO_5_NOEXCEPT = 0;

		//! Get the information about message type.
		virtual optional<std::type_index>
		msg_type() const SO_5_NOEXCEPT = 0;

		//! Get the information about message source.
		virtual optional<msg_source_t>
		msg_source() const SO_5_NOEXCEPT = 0;

		//! Get a pointer to agent from trace message.
		virtual optional<const so_5::agent_t *>
		agent() const SO_5_NOEXCEPT = 0;

		//! Get message or signal information.
		virtual optional<message_or_signal_flag_t>
		message_or_signal() const SO_5_NOEXCEPT = 0;

		//! Get message instance information.
		virtual optional<message_instance_info_t>
		message_instance_info() const SO_5_NOEXCEPT = 0;

		//! Get the description of a compound action.
		virtual optional<compound_action_description_t>
		compound_action() const SO_5_NOEXCEPT = 0;

		//! Get pointer to event handler.
		virtual optional<const so_5::impl::event_handler_data_t *>
		event_handler_data_ptr() const SO_5_NOEXCEPT = 0;
	};

//
// filter_t
//
/*!
 * \brief An interface of filter for trace messages.
 *
 * This interface must be implemented by all user-defined trace filters.
 *
 * \attention
 * User implementation of a trace filter must be thread-safe. Because
 * filter() method can be called on several threads at the save time.
 *
 * Usage example:
 * \code
 * // A filter for messages from specific mboxes/mchains.
 * class my_filter final : public so_5::msg_tracing::filter_t {
 * public:
 * 	using id_set = std::set<so_5::mbox_id_t>;
 *
 * 	my_filter(id_set ids) : ids_{std::move(ids)} {}
 *
 * 	virtual bool filter(const so_5::msg_tracing::trace_data_t & td) noexcept override {
 * 		const auto ms = td.msg_source();
 * 		if(ms) {
 * 			return 1 == ids_.count(ms->m_id);
 * 		}
 * 		else
 * 			return false;
 * 	}
 *
 * private:
 * 	id_set ids_;
 * };
 * ...
 * void some_agent::so_evt_start() {
 * 	my_filter::id_set ids{ ... };
 * 	so_environment().change_message_delivery_tracer_filter(
 * 		new my_filter{std::move(ids)} );
 * 	...
 * }
 * \endcode
 *
 * \note
 * In most cases it is much easier to use make_filter() helper function.
 *
 * \since
 * v.5.5.22
 */
class SO_5_TYPE filter_t : private so_5::atomic_refcounted_t
	{
		friend class so_5::intrusive_ptr_t<filter_t>;

	public :
		virtual ~filter_t() SO_5_NOEXCEPT = default;

		//! Filter the current message.
		/*!
		 * \return true if message should be placed into the trace.
		 */
		virtual bool
		filter(
			//! Accessor of trace data.
			const trace_data_t & data ) SO_5_NOEXCEPT = 0;
	};

//
// filter_shptr_t
//
/*!
 * \brief An alias for smart pointer to filter.
 *
 * \since
 * v.5.5.22
 */
using filter_shptr_t = intrusive_ptr_t< filter_t >;

namespace impl {

//
// filter_from_lambda_t
//
/*!
 * \brief A type of implementation of filters created from lambda function.
 *
 * \since
 * v.5.5.22
 */
template< typename L >
class filter_from_lambda_t : public filter_t
	{
		L m_lambda;

	public:
		filter_from_lambda_t( L lambda ) : m_lambda( std::move(lambda) ) {}

		virtual bool
		filter( const trace_data_t & data ) SO_5_NOEXCEPT
			{
				return m_lambda( data );
			}
	};

} /* namespace impl */

//
// make_filter
//
/*!
 * \brief A helper function for creation of new filter from lambda-function.
 *
 * In most cases usage of that function must easier than implementation
 * of a new class derived from filter_t interface.
 *
 * Usage example:
 * \code
 * void some_agent_t::so_evt_start() {
 * 	// Create a filter which will enable only messages from specific mboxes.
 * 	std::set<so_5::mbox_id_t> selected_ids{...};
 * 	so_environment().change_message_delivery_tracer_filter(
 * 		so_5::msg_tracing::make_filter( 
 * 			[ids = std::move(selected_ids)](so_5::msg_tracing::trace_data_t & td) {
 * 				const auto ms = td.msg_source();
 * 				return ms && 1 == ids.count(ms->m_id);
 * 			} ) );
 * 	...
 * }
 * \endcode
 *
 * \since
 * v.5.5.22
 */
template< typename L >
filter_shptr_t
make_filter( L && lambda )
	{
		using R = impl::filter_from_lambda_t< typename std::decay<L>::type >;

		return filter_shptr_t{ new R{ std::forward<L>(lambda) } };
	}

//
// make_enable_all_filter
//
/*!
 * \brief A helper function for creation of filter that enables all messages.
 *
 * Usage example:
 * \code
 * so_5::launch([](so_5::environment_t & env) {...},
 * 	[](so_5::environment_params_t & params) {
 * 		// Turn message delivery tracing on.
 * 		params.message_delivery_tracer(
 * 			so_5::msg_tracing::std_cout_tracer());
 * 		// Enable all trace messages.
 * 		params.message_delivery_tracer_filter(
 * 			so_5::msg_tracing::make_enable_all_filter());
 * 		...
 * 	} );
 * \endcode
 *
 * \since
 * v.5.5.22
 */
inline filter_shptr_t
make_enable_all_filter()
	{
		return make_filter( [](const trace_data_t &) { return true; } );
	}

//
// make_disable_all_filter
//
/*!
 * \brief A helper function for creation of filter that disables all messages.
 *
 * Usage example:
 * \code
 * so_5::launch([](so_5::environment_t & env) {...},
 * 	[](so_5::environment_params_t & params) {
 * 		// Turn message delivery tracing on.
 * 		params.message_delivery_tracer(
 * 			so_5::msg_tracing::std_cout_tracer());
 * 		// Disable all trace messages.
 * 		// It is expected that trace filter will be changed in the future.
 * 		params.message_delivery_tracer_filter(
 * 			so_5::msg_tracing::make_disable_all_filter());
 * 		...
 * 	} );
 * \endcode
 *
 * \since
 * v.5.5.22
 */
inline filter_shptr_t
make_disable_all_filter()
	{
		return make_filter( [](const trace_data_t &) { return false; } );
	}

//
// no_filter
//
/*!
 * \brief A helper function to be used when it is necessary to remove
 * msg_tracing's filter.
 *
 * Usage example:
 * \code
 * so_5::launch([](so_5::environment_t & env) {...},
 * 	[](so_5::environment_params_t & params) {
 * 		// Turn message delivery tracing on.
 * 		params.message_delivery_tracer(
 * 			so_5::msg_tracing::std_cout_tracer());
 * 		// Disable all trace messages.
 * 		// It is expected that trace filter will be changed in the future.
 * 		params.message_delivery_tracer_filter(
 * 			so_5::msg_tracing::make_disable_all_filter());
 * 		...
 * 	} );
 * ...
 * void some_agent_t::turn_msg_tracing_on() {
 * 	// Remove trace filter. As result all trace messages will be printed.
 * 	so_environment().change_message_delivery_tracer_filter(
 * 		so_5::msg_tracing::no_filter());
 * 	...
 * }
 * \endcode
 *
 * \since
 * v.5.5.22
 */
inline filter_shptr_t
no_filter() { return {}; }

//
// holder_t
//
/*!
 * \brief Interface of holder of message tracer and message trace filter objects.
 *
 * \since
 * v.5.5.22
 */
class SO_5_TYPE holder_t
	{
	public :
		holder_t() = default;
		virtual ~holder_t() SO_5_NOEXCEPT = default;

		holder_t(const holder_t &) = delete;
		holder_t & operator=(const holder_t &) = delete;

		//! Is message tracing enabled?
		virtual bool
		is_msg_tracing_enabled() const SO_5_NOEXCEPT = 0;

		//! Get access to the current message trace filter object.
		/*!
		 * \note
		 * This method should be called only if is_msg_tracing_enabled()
		 * returns true.
		 */
		virtual filter_shptr_t
		take_filter() SO_5_NOEXCEPT = 0;

		//! Get pointer to the message tracer object.
		/*!
		 * \note
		 * This method should be called only if is_msg_tracing_enabled()
		 * returns true.
		 */
		virtual tracer_t &
		tracer() const SO_5_NOEXCEPT = 0;
	};

} /* namespace msg_tracing */

} /* namespace so_5 */

