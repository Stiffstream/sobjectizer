/*
 * Check for enveloped service request.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

class special_wrapper_t : public so_5::enveloped_msg::envelope_t
{
	so_5::message_ref_t m_payload;

	std::atomic_bool m_enabled;

	void
	invoke_if_enabled( handler_invoker_t & invoker ) noexcept
	{
		if( m_enabled.load( std::memory_order_acquire ) )
			invoker.invoke( payload_info_t(m_payload) );
	}

public:
	special_wrapper_t( so_5::message_ref_t payload )
		:	m_payload( std::move(payload) )
	{
		m_enabled.store( true, std::memory_order_release );
	}

	void
	disable() noexcept
	{
		m_enabled.store( false, std::memory_order_release );
	}

	void
	access_hook(
		access_context_t,
		handler_invoker_t & invoker ) noexcept override
	{
		invoke_if_enabled( invoker );
	}
};

struct just_test_msg final : public so_5::message_t
{
	int m_v;

	just_test_msg( int v ) : m_v( v ) {}
};

void
run_test()
{
	so_5::wrapped_env_t sobj;

	auto mchain = create_mchain( sobj );

	so_5::message_ref_t msg1{ std::make_unique<just_test_msg>(0) };
	so_5::message_ref_t msg2{ std::make_unique<just_test_msg>(1) };

	so_5::intrusive_ptr_t< special_wrapper_t > env1{
			std::make_unique<special_wrapper_t>(msg1) };
	so_5::intrusive_ptr_t< special_wrapper_t > env2{
			std::make_unique<special_wrapper_t>(msg2) };

	mchain->as_mbox()->do_deliver_message(
			so_5::message_payload_type<just_test_msg>::subscription_type_index(),
			env1,
			1u );
	mchain->as_mbox()->do_deliver_message(
			so_5::message_payload_type<just_test_msg>::subscription_type_index(),
			env2,
			1u );

	env1->disable();

	std::size_t processed{};

	auto result = receive(
			from(mchain).handle_n(1).no_wait_on_empty(),
			[&](so_5::mhood_t<just_test_msg> cmd) {
				++processed;
				std::cout << "Msg: " << cmd->m_v << std::endl;
			} );

	const std::size_t expected = 1;

	ensure_or_die(
			expected == processed,
			"processed missmatch:\n"
			"expected=" + std::to_string(expected) + "\n"
			"  actual=" + std::to_string(processed) );

	ensure_or_die(
			expected == result.handled(),
			"handled missmatch:\n"
			"expected=" + std::to_string(expected) + "\n"
			"  actual=" + std::to_string(result.handled()) );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				run_test();
			},
			5 );
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

