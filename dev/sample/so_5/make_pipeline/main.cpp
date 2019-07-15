/*
 * An example of creating message processing pipeline from
 * a declarative description.
 *
 * This example consists from three parts:
 *
 * - the first one is a preparation of necessary infrastructure;
 * - the second one is a declaration of messages to be processed
 *   and all message's processing stuff;
 * - the third one is a declaration of pipeline and initiation of
 *   message processing.
 *
 * A task to be solved with the help of processing pipeline is
 * a processing of data samples from imaginary temperature sensor.
 *
 * A pipeline receives raw data sample on the input and does several
 * actions:
 * 
 * - validation of raw data;
 * - transformation of raw data to a temperature in Celsius degrees;
 * - archivation and distribution of converted value to outside world;
 * - checking value for allowed range;
 * - detection of dangerous situations when temperature is too high;
 * - initiation of alarm in presence of dangerous situation;
 * - distribution of the alarm.
 *
 * The main target of this example is showing how this processing stages could
 * be represented without manual description and creation of dedicated agents.
 *
 * It is possible. But some C++11 template magic must be used here.
 */

#include <iostream>
#include <cstdint>

#include <so_5/all.hpp>

using namespace std;

using namespace so_5;

/*
 * The first part.
 *
 * Definition of low-level pipeline implementation details.
 */

//
// All messages will be passed as SObjectizer-messages.
// It means that they will be created as dynamically allocated objects.
// The name stage_result_t will be used as alias for smart-pointer to
// dynamically created message.
//
// The name stage_result_t means that messages to be passed will be
// returned as stage processing result.
//
template< typename M >
using stage_result_t = message_holder_t< M >;

//
// Just a helper function for creating new message instance.
// Something like std::make_shared or std::make_unique.
//
template< typename M, typename... Args >
stage_result_t< M >
make_result( Args &&... args )
{
	return stage_result_t< M >::make(forward< Args >(args)...);
}

//
// Just a helper function for the case when there is no processing result
// (stage has to return nothing).
//
template< typename M >
stage_result_t< M >
make_empty()
{
	return stage_result_t< M >();
}

//
// We have to deal with two types of stage handlers:
// - intermediate handlers which will return some result (e.g. some new
//   message);
// - terminal handlers which can return nothing (e.g. void instead of
//   stage_result_t<M>);
//
// This template with specialization defines `input` and `output`
// aliases for both cases.
//
template< typename In, typename Out >
struct handler_traits_t
{
	using input = In;
	using output = stage_result_t< Out >;
};

template< typename In >
struct handler_traits_t< In, void >
{
	using input = In;
	using output = void;
};

//
// A template for repesentation of one stage handler.
//
// The handler could be specified as free function pointer, as stateful
// object with operator() or as lambda function. The actual handler will
// be stored as instance of std::function.
//
template< typename In, typename Out >
class stage_handler_t
{
public :
	using traits = handler_traits_t< In, Out >;
	using func_type = function< typename traits::output(const typename traits::input &) >;

	stage_handler_t( func_type handler )
		: m_handler( move(handler) )
	{}

	template< typename Callable >
	stage_handler_t( Callable handler ) : m_handler( handler ) {}

	typename traits::output
	operator()( const typename traits::input & a ) const
	{
		return m_handler( a );
	}

private :
	func_type m_handler;
};

//
// An agent which will be used as intermediate or terminal pipeline stage.
// It will receive input message, call the stage handler and pass
// handler result to the next stage (if any).
//
template< typename In, typename Out >
class a_stage_point_t final : public agent_t
{
public :
	a_stage_point_t(
		context_t ctx,
		stage_handler_t< In, Out > handler,
		mbox_t next_stage )
		:	agent_t{ ctx }
		,	m_handler{ move( handler ) }
		,	m_next{ move(next_stage) }
	{}

	void so_define_agent() override
	{
		if( m_next )
			// Because there is the next stage the appropriate
			// message handler will be used.
			so_subscribe_self().event( [=]( const In & evt ) {
					auto r = m_handler( evt );
					if( r )
						so_5::send( m_next, r );
				} );
		else
			// There is no next stage. A very simple message handler
			// will be used for that case.
			so_subscribe_self().event( [=]( const In & evt ) {
					m_handler( evt );
				} );
	}

private :
	const stage_handler_t< In, Out > m_handler;
	const mbox_t m_next;
};

//
// A specialization of a_stage_point_t for the case of terminal stage of
// a pipeline. This type will be used for stage handlers with void
// return type.
//
template< typename In >
class a_stage_point_t< In, void > final : public agent_t
{
public :
	a_stage_point_t(
		context_t ctx,
		stage_handler_t< In, void > handler,
		mbox_t next_stage )
		:	agent_t{ ctx }
		,	m_handler{ move( handler ) }
	{
		if( next_stage )
			throw std::runtime_error( "sink point cannot have next stage" );
	}

	void so_define_agent() override
	{
		so_subscribe_self().event( [=]( const In & evt ) {
				m_handler( evt );
			} );
	}

private :
	const stage_handler_t< In, void > m_handler;
};

//
// An agent type for such special case as broadcasting of a message
// to several parallel and independent pipelines. An agent receives
// an input messages and resends it to every pipeline specified.
//
// An agent of such type should be used only for terminal stages.
//
template< typename In >
class a_broadcaster_t final : public agent_t
{
public :
	a_broadcaster_t(
		context_t ctx,
		vector< mbox_t > && next_stages )
		:	agent_t{ ctx }
		,	m_next_stages( move(next_stages) )
	{}

	void so_define_agent() override
	{
		so_subscribe_self().event( &a_broadcaster_t::evt_broadcast );
	}

private :
	const vector< mbox_t > m_next_stages;

	void evt_broadcast( mhood_t< In > evt )
	{
		// The same message instance will be redirected to subsequent stages.
		for( const auto & mbox : m_next_stages )
			so_5::send( mbox, evt );
	}
};

//
// An alias for functional object to build necessary agent for a
// pipeline stage.
//
using stage_builder_t = function< mbox_t(coop_t &, mbox_t) >;

//
// Description of one pipeline stage.
//
template< typename In, typename Out >
struct stage_t
{
	stage_builder_t m_builder;
};

//
// A bunch of definitions related to detection of handler argument
// type and return value.
//

// 
// Helper type for `arg_type` and `result_type` alises definition.
//
template< typename R, typename A >
struct callable_traits_typedefs_t
{
	using arg_type = A;
	using result_type = R;
};

//
// Helper type for dealing with statefull objects with operator()
// (they could be usef-defined objects or generated by compiler
// like lambdas).
//
template< typename T >
struct lambda_traits_t;

template< typename M, typename A, typename T >
struct lambda_traits_t< stage_result_t< M >(T::*)(const A &) const >
	:	public callable_traits_typedefs_t< M, A >
{};

template< typename A, typename T >
struct lambda_traits_t< void (T::*)(const A &) const >
	:	public callable_traits_typedefs_t< void, A >
{};

template< typename M, typename A, typename T >
struct lambda_traits_t< stage_result_t< M >(T::*)(const A &) >
	:	public callable_traits_typedefs_t< M, A >
{};

template< typename A, typename T >
struct lambda_traits_t< void (T::*)(const A &) >
	:	public callable_traits_typedefs_t< void, A >
{};

//
// Main type for definition of `arg_type` and `result_type` aliases.
// With specialization for various cases.
//
template< typename T >
struct callable_traits_t
	:	public lambda_traits_t< decltype(&T::operator()) >
{};

template< typename M, typename A >
struct callable_traits_t< stage_result_t< M >(*)(const A &) >
	:	public callable_traits_typedefs_t< M, A >
{};

template< typename A >
struct callable_traits_t< void(*)(const A &) >
	:	public callable_traits_typedefs_t< void, A >
{};

//
// Main function for defining intermediate of terminal stage of a pipeline.
//
template<
	typename Callable,
	typename In = typename callable_traits_t< Callable >::arg_type,
	typename Out = typename callable_traits_t< Callable >::result_type >
stage_t< In, Out >
stage( Callable handler )
{
	stage_builder_t builder{
			[h = std::move(handler)](
				coop_t & coop,
				mbox_t next_stage) -> mbox_t
			{
				return coop.make_agent< a_stage_point_t<In, Out> >(
						std::move(h),
						std::move(next_stage) )
					->so_direct_mbox();
			}
	};

	return { std::move(builder) };
}
	
//
// Serie of helper functions for building description for
// `broadcast` stage.
//
// Those functions are used for collecting
// `builders` functions for every child pipeline.
//
template< typename In, typename Out, typename... Rest >
void
move_sink_builder_to(
	vector< stage_builder_t > & receiver,
	stage_t< In, Out > && first,
	Rest &&... rest )
{
	receiver.emplace_back( move( first.m_builder ) );
	if constexpr( 0u != sizeof...(rest) )
		move_sink_builder_to<In>( receiver, forward< Rest >(rest)... );
}

template< typename In, typename Out, typename... Rest >
vector< stage_builder_t >
collect_sink_builders( stage_t< In, Out > && first, Rest &&... stages )
{
	vector< stage_builder_t > receiver;
	receiver.reserve( 1 + sizeof...(stages) );
	move_sink_builder_to<In>(
			receiver,
			move(first),
			std::forward<Rest>(stages)... );

	return receiver;
}

//
// Main helper function for building `broadcast` stage.
//
template< typename In, typename Out, typename... Rest >
stage_t< In, void >
broadcast( stage_t< In, Out > && first, Rest &&... stages )
{
	stage_builder_t builder{
		[broadcasts = collect_sink_builders(
				move(first), forward< Rest >(stages)...)]
		( coop_t & coop, mbox_t ) -> mbox_t
		{
			vector< mbox_t > mboxes;
			mboxes.reserve( broadcasts.size() );

			for( const auto & b : broadcasts )
				mboxes.emplace_back( b( coop, mbox_t{} ) );

			return coop.make_agent< a_broadcaster_t<In> >( std::move(mboxes) )
					->so_direct_mbox();
		}
	};

	return { std::move(builder) };
}

//
// Helper `operator|` for continuation of a pipeline definition.
//
template< typename In, typename Out1, typename Out2 >
stage_t< In, Out2 >
operator|(
	stage_t< In, Out1 > && prev,
	stage_t< Out1, Out2 > && next )
{
	return {
		stage_builder_t{
			[prev, next]( coop_t & coop, mbox_t next_stage ) -> mbox_t
			{
				auto m = next.m_builder( coop, std::move(next_stage) );
				return prev.m_builder( coop, std::move(m) );
			}
		}
	};
}

//
// Main function for creation of all pipeline-related stuff.
//
template< typename In, typename Out, typename... Args >
mbox_t
make_pipeline(
	// Agent who will own a cooperation with pipeline-related agent.
	agent_t & owner,
	// Definition of a pipeline.
	stage_t< In, Out > && sink,
	// Optional args to be passed to so_5::create_child_coop function.
	Args &&... args )
{
	auto coop = create_child_coop( owner, forward< Args >(args)... );

	auto mbox = sink.m_builder( *coop, mbox_t{} );

	owner.so_environment().register_coop( move(coop) );

	return mbox;
}

/*
 * The second part.
 *
 * Definition of messages to be processed by a pipeline and
 * the message processing code.
 */

// Raw data from a sensor.
struct raw_measure
{
	int m_meter_id;

	uint8_t m_high_bits;
	uint8_t m_low_bits;
};

// Type of input for validation stage with raw data from a sensor.
struct raw_value
{
	raw_measure m_data;
};

// Type of input for conversion stage with valid raw data from a sensor.
struct valid_raw_value
{
	raw_measure m_data;
};

// Data from a sensor after conversion to Celsius degrees.
struct calculated_measure
{
	int m_meter_id;

	float m_measure;
};

// The type for result of conversion stage with converted data from a sensor.
struct sensor_value
{
	calculated_measure m_data;
};

// Type with value which could mean a dangerous level of temperature.
struct suspicional_value
{
	calculated_measure m_data;
};

// Type with information about detected dangerous situation.
struct alarm_detected
{
	int m_meter_id;
};

//
// The first stage of a pipeline. Validation of raw data from a sensor.
//
// Returns valid_raw_value or nothing if value is invalid.
//
stage_result_t< valid_raw_value >
validation( const raw_value & v )
{
	if( 0x7 >= v.m_data.m_high_bits )
		return make_result< valid_raw_value >( v.m_data );
	else
		return make_empty< valid_raw_value >();
}

//
// The second stage of a pipeline. Conversion from raw data to a value
// in Celsius degrees.
//
stage_result_t< sensor_value >
conversion( const valid_raw_value & v )
{
	return make_result< sensor_value >(
		calculated_measure{ v.m_data.m_meter_id,
			0.5f * ((static_cast< uint16_t >( v.m_data.m_high_bits ) << 8) +
				v.m_data.m_low_bits) } );
}

//
// Simulation of the data archivation.
//
void
archivation( const sensor_value & v )
{
	clog << "archiving (" << v.m_data.m_meter_id << ","
		<< v.m_data.m_measure << ")" << endl;
}

//
// Simulation of the data distribution.
//
void
distribution( const sensor_value & v )
{
	clog << "distributing (" << v.m_data.m_meter_id << ","
		<< v.m_data.m_measure << ")" << endl;
}

//
// The first stage of a child pipeline at third level of the main pipeline.
//
// Checking for to high value of the temperature.
//
// Returns suspicional_value message or nothing.
//
stage_result_t< suspicional_value >
range_checking( const sensor_value & v )
{
	if( v.m_data.m_measure >= 45.0f )
		return make_result< suspicional_value >( v.m_data );
	else
		return make_empty< suspicional_value >();
}

//
// The next stage of a child pipeline.
//
// Checks for two suspicional_value-es in 25ms time window.
//
class alarm_detector
{
	using clock = chrono::steady_clock;

public :
	stage_result_t< alarm_detected >
	operator()( const suspicional_value & v )
	{
		if( m_previous )
			if( *m_previous + chrono::milliseconds(25) > clock::now() )
			{
				m_previous = std::nullopt;
				return make_result< alarm_detected >( v.m_data.m_meter_id );
			}

		m_previous = clock::now();
		return make_empty< alarm_detected >();
	}

private :
	optional< clock::time_point > m_previous;
};

//
// One of last stages of a child pipeline.
// Imitates beginning of the alarm processing.
//
void
alarm_initiator( const alarm_detected & v )
{
	clog << "=== alarm (" << v.m_meter_id << ") ===" << endl;
}

//
// Another of last stages of a child pipeline.
// Imitates distribution of the alarm.
//
void
alarm_distribution( ostream & to, const alarm_detected & v )
{
	to << "alarm_distribution (" << v.m_meter_id << ")" << endl;
}


/*
 * The third part.
 *
 * Definition of message processing pipeline and imitation of
 * several measures from a sensor.
 */

class a_parent_t final : public agent_t
{
	// Signal for finishing the imitation.
	struct shutdown : public signal_t {};

public :
	a_parent_t( context_t ctx ) : agent_t( ctx )
	{}

	void so_define_agent() override
	{
		// On shutdown the coop and its children must be deregistered.
		so_subscribe_self().event(
				[this](mhood_t< shutdown >) { so_deregister_agent_coop_normally(); } );
	}

	void so_evt_start() override
	{
		// Construction of a pipeline.
		auto pipeline = make_pipeline( *this,
				stage(validation) | stage(conversion) | broadcast(
					stage(archivation),
					stage(distribution),
					stage(range_checking) | stage(alarm_detector{}) | broadcast(
						stage(alarm_initiator),
						stage( []( const alarm_detected & v ) {
								alarm_distribution( cerr, v );
							} )
						)
					) );

		// One second for imitation then shutdown.
		send_delayed< shutdown >( *this, chrono::seconds(1) );

		// Imitation of several samples from a sensor.
		// One sample for each 10ms.
		for( uint8_t i = 0; i < static_cast< uint8_t >(250); i += 10 )
			send_delayed< raw_value >(
					pipeline,
					chrono::milliseconds( i ),
					raw_measure{ 0, 0, i } );
	}
};

int main()
{
	try
	{
		so_5::launch( []( environment_t & env ) {
				env.register_agent_as_coop( env.make_agent< a_parent_t >() );
			} );
	}
	catch( const exception & x )
	{
		cerr << "Exception: " << x.what() << endl;
	}
}

