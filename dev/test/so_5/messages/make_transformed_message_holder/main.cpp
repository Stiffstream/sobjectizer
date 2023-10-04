/*
 * Tests for use message_holder_t with make_transformed.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <tuple>

namespace test
{

struct classical_message_t final : public so_5::message_t
	{
		int m_a;

		explicit classical_message_t( int a ) : m_a{ a } {}
	};

struct user_defined_message_t
	{
		int m_a;

		explicit user_defined_message_t( int a ) : m_a{ a } {}
	};

void
ensure_expected_value( int expected, int actual )
	{
		if( expected != actual )
			throw std::runtime_error{ std::to_string( expected ) + " != "
					+ std::to_string( actual ) };
	}

template< typename T >
[[nodiscard]]
auto &
extract_payload_ptr( so_5::message_ref_t msg_ref )
	{
		auto * p = so_5::message_payload_type< T >::extract_payload_ptr( msg_ref );
		if( !p )
			throw std::runtime_error{ "payload extraction failed" };
		return *p;
	}

void
run_inside_sobjectizer( so_5::environment_t & env )
	{
		constexpr int expected_v = 5436;

		const auto dummy_mbox = env.create_mbox();

		//
		// classical message
		//
		{
			using T = classical_message_t;
			auto msg = so_5::message_holder_t< T >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = classical_message_t;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::shared >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = classical_message_t;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::unique >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::immutable_msg< classical_message_t >;
			auto msg = so_5::message_holder_t< T >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::immutable_msg< classical_message_t >;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::shared >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::immutable_msg< classical_message_t >;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::unique >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::mutable_msg< classical_message_t >;
			auto msg = so_5::message_holder_t< T >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::mutable_msg< classical_message_t >;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::shared >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::mutable_msg< classical_message_t >;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::unique >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		//
		// user defined message
		//
		{
			using T = user_defined_message_t;
			auto msg = so_5::message_holder_t< T >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = user_defined_message_t;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::shared >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = user_defined_message_t;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::unique >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::immutable_msg< user_defined_message_t >;
			auto msg = so_5::message_holder_t< T >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::immutable_msg< user_defined_message_t >;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::shared >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::immutable_msg< user_defined_message_t >;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::unique >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::mutable_msg< user_defined_message_t >;
			auto msg = so_5::message_holder_t< T >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::mutable_msg< user_defined_message_t >;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::shared >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}

		{
			using T = so_5::mutable_msg< user_defined_message_t >;
			auto msg = so_5::message_holder_t<
					T,
					so_5::message_ownership_t::unique >::make( expected_v );
			auto r = so_5::make_transformed( dummy_mbox, std::move(msg) );
			ensure_expected_value( expected_v,
					extract_payload_ptr< T >( r.message() ).m_a );
		}
	}

} /* namespace test */

int
main()
{
	using namespace test;

	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( run_inside_sobjectizer /*,
					[]( so_5::environment_params_t & p ) {
						p.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
					}*/ );
			},
			5 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

