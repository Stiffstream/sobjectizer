/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Interfaces for work_thread and work_thread's factory.
 * \since v.5.7.3
 */

#include <so_5/disp/abstract_work_thread.hpp>

#include <thread>

namespace so_5::disp
{

//
// abstract_work_thread_t
//
abstract_work_thread_t::abstract_work_thread_t()
	{}

abstract_work_thread_t::~abstract_work_thread_t()
	{}

//
// abstract_work_thread_factory_t
//
abstract_work_thread_factory_t::abstract_work_thread_factory_t()
	{}
abstract_work_thread_factory_t::~abstract_work_thread_factory_t()
	{}

namespace std_work_thread_impl
{

//
// std_work_thread_t
//
//FIXME: document this!
class std_work_thread_t : public abstract_work_thread_t
	{
		//! Actual thread.
		std::thread m_thread;

	public:
		std_work_thread_t() = default;

		void
		start( body_func_t thread_body ) override
			{
				m_thread = std::thread{ std::move(thread_body) };
			}

		void
		join() override
			{
				// Do not check m_thread status because join() has to be called
				// only once and only if there was previous call to start(),
				// and that call was successful.
				m_thread.join();
			}
	};

//
// std_work_thread_factory_t
//
//FIXME: document this!
class std_work_thread_factory_t : public abstract_work_thread_factory_t
	{
	public:
		std_work_thread_factory_t() = default;

		[[nodiscard]]
		abstract_work_thread_t &
		acquire( so_5::environment_t & /*env*/ ) override
			{
				return *(new std_work_thread_t{});
			}

		void
		release( abstract_work_thread_t & thread ) noexcept override
			{
				// Assume that 'thread' was created via acquire() method,
				// so we can safely delete it.
				delete (&thread);
			}
	};

} /* namespace std_work_thread_impl */

//
// make_std_work_thread_factory
//
[[nodiscard]]
SO_5_FUNC
abstract_work_thread_factory_shptr_t
make_std_work_thread_factory()
{
	return std::make_shared<
			std_work_thread_impl::std_work_thread_factory_t >();
}

} /* namespace so_5::disp */

