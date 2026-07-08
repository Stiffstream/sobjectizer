// Simple test for top_level_task_t type from this_thread_scheduler.

#include <so_5/cpp_coro/this_thread_scheduler.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

namespace test
{

class single_int_task_t
	{
	public:
		struct promise_type
			{
				int m_value{};

				std::coroutine_handle<> m_continuation;

				std::suspend_always
				initial_suspend() const noexcept { return {}; }

				class final_awaiter_t
				{
					std::coroutine_handle<> m_continuation;

				public:
					final_awaiter_t(
						std::coroutine_handle<> continuation ) noexcept
						: m_continuation{ std::move(continuation) }
					{}

					[[nodiscard]]
					bool
					await_ready() const noexcept { return false; }

					[[nodiscard]]
					std::coroutine_handle<>
					await_suspend( std::coroutine_handle<> ) const noexcept
						{
							return m_continuation;
						}

					void
					await_resume() const noexcept {}
				};

				final_awaiter_t
				final_suspend() const noexcept
					{
						return { m_continuation };
					}

				[[nodiscard]]
				single_int_task_t
				get_return_object() noexcept
					{
						return single_int_task_t{
								std::coroutine_handle< promise_type >
										::from_promise( *this )
							};
					}

				void
				return_value( int v ) noexcept
					{
						m_value = v;
					}

				void
				unhandled_exception()
					{
						throw;
					}
			};

	private:
		std::coroutine_handle< promise_type > m_coro;

		single_int_task_t(
			std::coroutine_handle< promise_type > coro ) noexcept
			: m_coro{ coro }
			{}

	public:
		~single_int_task_t()
		{
			if( m_coro )
				m_coro.destroy();
		}

		single_int_task_t( const single_int_task_t & ) = delete;
		single_int_task_t &
		operator=( const single_int_task_t & ) = delete;

		single_int_task_t( single_int_task_t && other ) noexcept
			: m_coro{ std::exchange(
					other.m_coro,
					std::coroutine_handle< promise_type >() ) }
			{}

		single_int_task_t &
		operator=( single_int_task_t && other ) noexcept
			{
				if( this != std::addressof(other) )
				{
					if( m_coro ) m_coro.destroy();

					m_coro = std::exchange(
							other.m_coro,
							std::coroutine_handle< promise_type >() );
				}

				return *this;
			}

		bool
		await_ready() const noexcept { return false; }

		std::coroutine_handle<>
		await_suspend( std::coroutine_handle<> parent )
			{
				m_coro.promise().m_continuation = parent;
				return m_coro;
			}

		int
		await_resume() const noexcept
			{
				return m_coro.promise().m_value;
			}
	};

single_int_task_t
get_int_value()
	{
		co_return 42;
	};

so_5::cpp_coro::this_thread_scheduler_impl::top_level_task_t< int >
int_case()
	{
		co_return co_await get_int_value();
	}

void
check_int_case()
	{
		auto t = int_case();
		t.m_coro.resume();
		ensure_or_die( 42 == t.await_resume(), "42 is expected" );
	}

class single_void_task_t
	{
	public:
		struct promise_type
			{
				std::coroutine_handle<> m_continuation;

				std::suspend_always
				initial_suspend() const noexcept { return {}; }

				class final_awaiter_t
				{
					std::coroutine_handle<> m_continuation;

				public:
					final_awaiter_t(
						std::coroutine_handle<> continuation ) noexcept
						: m_continuation{ std::move(continuation) }
					{}

					[[nodiscard]]
					bool
					await_ready() const noexcept { return false; }

					[[nodiscard]]
					std::coroutine_handle<>
					await_suspend( std::coroutine_handle<> ) const noexcept
						{
							return m_continuation;
						}

					void
					await_resume() const noexcept {}
				};

				final_awaiter_t
				final_suspend() const noexcept
					{
						return { m_continuation };
					}

				[[nodiscard]]
				single_void_task_t
				get_return_object() noexcept
					{
						return single_void_task_t{
								std::coroutine_handle< promise_type >
										::from_promise( *this )
							};
					}

				void
				return_void() noexcept
					{}

				void
				unhandled_exception()
					{
						throw;
					}
			};

	private:
		std::coroutine_handle< promise_type > m_coro;

		single_void_task_t(
			std::coroutine_handle< promise_type > coro ) noexcept
			: m_coro{ coro }
			{}

	public:
		~single_void_task_t()
		{
			if( m_coro )
				m_coro.destroy();
		}

		single_void_task_t( const single_void_task_t & ) = delete;
		single_void_task_t &
		operator=( const single_void_task_t & ) = delete;

		single_void_task_t( single_void_task_t && other ) noexcept
			: m_coro{ std::exchange(
					other.m_coro,
					std::coroutine_handle< promise_type >() ) }
			{}

		single_void_task_t &
		operator=( single_void_task_t && other ) noexcept
			{
				if( this != std::addressof(other) )
				{
					if( m_coro ) m_coro.destroy();

					m_coro = std::exchange(
							other.m_coro,
							std::coroutine_handle< promise_type >() );
				}

				return *this;
			}

		bool
		await_ready() const noexcept { return false; }

		std::coroutine_handle<>
		await_suspend( std::coroutine_handle<> parent )
			{
				m_coro.promise().m_continuation = parent;
				return m_coro;
			}

		void
		await_resume() const noexcept {}
	};

single_void_task_t
get_void_value( int & marker )
	{
		marker = 42;
		co_return;
	};

so_5::cpp_coro::this_thread_scheduler_impl::top_level_task_t< void >
void_case( int & marker )
	{
		co_return co_await get_void_value( marker );
	}

void
check_void_case()
	{
		int marker = 11;
		auto t = void_case( marker );
		t.m_coro.resume();
		ensure_or_die( 42 == marker, "42 is expected" );
	}

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				check_int_case();
				check_void_case();
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
