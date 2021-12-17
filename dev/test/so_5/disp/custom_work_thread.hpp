/*
 * SObjectizer-5
 */

#pragma once

#include <so_5/all.hpp>

#include <atomic>
#include <iostream>
#include <thread>

namespace disp_tests
{

class custom_work_thread_t final : public so_5::disp::abstract_work_thread_t
	{
		std::atomic<unsigned int> & m_started_counter;
		std::atomic<unsigned int> & m_finished_counter;

		std::thread m_thread;

	public:
		custom_work_thread_t(
			std::atomic<unsigned int> & started_counter,
			std::atomic<unsigned int> & finished_counter )
			:	m_started_counter{ started_counter }
			,	m_finished_counter{ finished_counter }
			{}

		void
		start( body_func_t thread_body ) override
		{
			m_thread = std::thread{ [this, tb = std::move(thread_body)]() {
					++m_started_counter;
					tb();
					++m_finished_counter;
				} };
		}

		void
		join() override
		{
			m_thread.join();
		}
	};

class custom_work_thread_factory_t final
	: public so_5::disp::abstract_work_thread_factory_t
	{
		std::atomic<unsigned int> m_started_count{};
		std::atomic<unsigned int> m_finished_count{};
		std::atomic<unsigned int> m_created_count{};
		std::atomic<unsigned int> m_destroyed_count{};

	public:
		custom_work_thread_factory_t() = default;

		so_5::disp::abstract_work_thread_t &
		acquire( so_5::environment_t & /*env*/ ) override
			{
				auto * thr = new custom_work_thread_t{
						m_started_count,
						m_finished_count
					};
				++m_created_count;
				return *thr;
			}

		void
		release( so_5::disp::abstract_work_thread_t & thread ) noexcept override
			{
				++m_destroyed_count;
				delete &thread;
			}

		[[nodiscard]]
		auto
		started() const noexcept
		{
			return m_started_count.load( std::memory_order_acquire );
		}

		[[nodiscard]]
		auto
		finished() const noexcept
		{
			return m_finished_count.load( std::memory_order_acquire );
		}

		[[nodiscard]]
		auto
		created() const noexcept
		{
			return m_created_count.load( std::memory_order_acquire );
		}

		[[nodiscard]]
		auto
		destroyed() const noexcept
		{
			return m_destroyed_count.load( std::memory_order_acquire );
		}
	};

} /* disp_tests */

