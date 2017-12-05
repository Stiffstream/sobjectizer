/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.5
 *
 * \file
 * \brief All stuff related to tuple_as_message.
 */

#pragma once

#include <so_5/rt/h/message.hpp>

#include <tuple>

namespace so_5 {

#if defined( _MSC_VER ) && (_MSC_VER <= 1800)
	#pragma warning(push)
	#pragma warning(disable: 4520)
#endif

/*!
 * \since
 * v.5.5.5
 *
 * \brief A template which allows to use tuples as messages.
 *
 * \tparam Tag a type tag for distinguish messages with the same fields list.
 * \tparam Types types for message fields.
 *
 * This template is added to allow a user to use simple constructs for
 * very simple messages, when there is no need to define a full class
 * for message.
 *
 * Just for comparison:

	\code
	// This is recommended way to defining of messages.
	// Separate class allows to easily extend or refactor
	// message type in the future.
	struct process_data : public so_5::message_t
	{
		const std::uint8_t * m_data;

		// Constructor is necessary.
		process_data( const std::uint8_t * data )
			:	m_data( data )
		{}
	};

	// And the event-handler for this message.
	void data_processing_agent::evt_process_data( const process_data & evt )
	{
		do_data_processing( evt.m_data ); // Some processing 
	}
	\endcode

 * And this the one possible usage of tuple_as_message_t for the same task:

	\code
	// This way of defining messages is not recommended for big projects
	// with large code base and big amount of agents and messages.
	// But can be useful for small to-be-thrown-out utilities.
	struct process_data_tag {};
	using process_data = so_5::tuple_as_message_t< process_data_tag, const std::uint8_t * >;

	// And the event-handler for this message.
	void data_processing_agent::evt_process_data( const process_data & evt )
	{
		do_data_processing( std::get<0>( evt ) );
	}
	\endcode

 * Or even yet more simplier and dirtier:

	\code
	// This way of defining messages can be good only for very and very
	// small to-be-thrown-out programs (like tests and samples).
	using process_data = so_5::tuple_as_message_t<
				std::integral_constant<int, 0>, const std::uint8_t * >;

	// And the event-handler for this message.
	void data_processing_agent::evt_process_data( const process_data & evt )
	{
		do_data_processing( std::get<0>( evt ) );
	}
	\endcode
 */
template< typename Tag, typename... Types >
struct tuple_as_message_t
	:	public std::tuple< Types... >
	,	public so_5::message_t
{
	using base_tuple_type = std::tuple< Types... >;

	tuple_as_message_t() 
		{}

	tuple_as_message_t( const tuple_as_message_t & ) = default;

	tuple_as_message_t &
	operator=( const tuple_as_message_t & ) = default;

#if !defined( _MSC_VER )
	tuple_as_message_t( tuple_as_message_t && ) = default;

	tuple_as_message_t &
	operator=( tuple_as_message_t && ) = default;
#endif

	explicit tuple_as_message_t( const Types &... args )
		:	base_tuple_type( args... )
	{}

	template< typename... Utypes >
	tuple_as_message_t( Utypes &&... args )
		:	base_tuple_type( std::forward< Utypes >( args )... )
	{}
};

#if defined( _MSC_VER ) && (_MSC_VER <= 1800)
	#pragma warning(pop)
#endif

//
// mtag
//
/*!
 * \since
 * v.5.5.5
 *
 * \brief A helper template for simplification of defining unique message tags
 * for tuple_as_message.
 *
 * \par Usage sample:
	\code
	using namespace std;

	// Defining domain specific messages as tuples.
	using process_range = tuple_as_message_t< mtag<0>, string, string >;
	using success_result = tuple_as_message_t< mtag<1>, string >;
	using processing_failes = tuple_as_message_t< mtag<2>, int, string >;
	\endcode
 *
 * This is a short equivalent of:
	\code
	using namespace std;

	// Defining domain specific messages as tuples.
	using process_range = tuple_as_message_t<
		integral_constant<0>, string, string >;
	using success_result = tuple_as_message_t<
		integral_constant<1>, string >;
	using processing_failes = tuple_as_message_t<
		integral_constant<2>, int, string >;
	\endcode
 */
template< int N >
using mtag = std::integral_constant< int, N >;

//
// typed_mtag
//
/*!
 * \since
 * v.5.5.5
 *
 * \brief A helper template for simplification of defining unique message tags
 * for tuple as message.
 *
 * Very similar to so_5::mtag but allows to define message tags for
 * different modules.
 *
 * \par Usage sample:
	\code
	using namespace std;

	// The first module.
	namespace first_module {

	// Type unique for that module.
	struct tag {};

	// Messages for that module.
	using process_range = tuple_as_message_t< typed_mtag<tag, 0>, string, string >;
	using success_result = tuple_as_message_t< typed_mtag<tag, 1>, string >;
	using processing_failes = tuple_as_message_t< typed_mtag<tag, 2>, int, string >;
	} // namespace first_module.

	// The second module.
	namespace second_module {

	// Type unique for that module.
	struct tag {};

	// Messages for that module.
	using process_range = tuple_as_message_t< typed_mtag<tag, 0>, string, string >;
	using success_result = tuple_as_message_t< typed_mtag<tag, 1>, string >;
	using processing_failes = tuple_as_message_t< typed_mtag<tag, 2>, int, string >;

	} // namespace second_module
	\endcode
 * By the help of typed_mtag messages first_module::process_range and
 * second_module::process_range will be different and will have different
 * typeid.
 */
template< typename T, int N >
struct typed_mtag {};

namespace rt {

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::tuple_as_message_t
 * instead.
 */
template< typename Tag, typename... Types >
using tuple_as_message_t = so_5::tuple_as_message_t< Tag, Types... >;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::mtag instead.
 */
template< int N >
using mtag = so_5::mtag< N >;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::typed_mtag instead.
 */
template< typename T, int N >
using typed_mtag = so_5::typed_mtag< T, N >;

} /* namespace rt */

} /* namespace so_5 */

