/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Interfaces for work_thread and work_thread's factory.
 * \since v.5.7.3
 */

#pragma once

#include <so_5/declspec.hpp>
#include <so_5/fwd.hpp>

#include <functional>
#include <memory>

namespace so_5::disp
{

//
// abstract_work_thread_t
//
//FIXME: document this!
class SO_5_TYPE abstract_work_thread_t
	{
	public:
		//FIXME: document this!
		using body_func_t = std::function< void() >;

		abstract_work_thread_t();

		abstract_work_thread_t( const abstract_work_thread_t & ) = delete;
		abstract_work_thread_t &
		operator=( const abstract_work_thread_t & ) = delete;

		abstract_work_thread_t( abstract_work_thread_t && ) = delete;
		abstract_work_thread_t &
		operator=( abstract_work_thread_t && ) = delete;

		virtual ~abstract_work_thread_t();

		//FIXME: document this!
		virtual void
		start( body_func_t thread_body ) = 0;

		//FIXME: document this!
		//FIXME: this method isn't noexcept, that should be mentioned in docs.
		virtual void
		join() = 0;
	};

//
// abstract_work_thread_factory_t
//
//FIXME: document this!
class SO_5_TYPE abstract_work_thread_factory_t
	{
	public:
		abstract_work_thread_factory_t();

		abstract_work_thread_factory_t(
				const abstract_work_thread_factory_t & ) = delete;
		abstract_work_thread_factory_t &
		operator=( const abstract_work_thread_factory_t & ) = delete;

		abstract_work_thread_factory_t(
				abstract_work_thread_factory_t && ) = delete;
		abstract_work_thread_factory_t &
		operator=( abstract_work_thread_factory_t && ) = delete;

		virtual ~abstract_work_thread_factory_t();

		//FIXME: document this!
		[[nodiscard]]
		virtual abstract_work_thread_t &
		acquire( so_5::environment_t & env ) = 0;

		//FIXME: document this!
		virtual void
		release( abstract_work_thread_t & thread ) noexcept = 0;
	};

//
// abstract_work_thread_factory_shptr_t
//
//FIXME: document this!
using abstract_work_thread_factory_shptr_t = std::shared_ptr<
		abstract_work_thread_factory_t >;

//
// work_thread_holder_t
//
//FIXME: document this!
class [[nodiscard]] work_thread_holder_t
	{
		abstract_work_thread_t * m_thread{};
		abstract_work_thread_factory_shptr_t m_factory{};

	public:
		friend void
		swap( work_thread_holder_t & a, work_thread_holder_t & b ) noexcept
			{
				using std::swap;

				swap( a.m_thread, b.m_thread );
				swap( a.m_factory, b.m_factory );
			}

		work_thread_holder_t() noexcept = default;

		work_thread_holder_t(
			abstract_work_thread_t & thread,
			abstract_work_thread_factory_shptr_t factory ) noexcept
			:	m_thread{ &thread }
			,	m_factory{ std::move(factory) }
			{}

		work_thread_holder_t( const work_thread_holder_t & ) = delete;
		work_thread_holder_t &
		operator=( const work_thread_holder_t & ) = delete;

		work_thread_holder_t( work_thread_holder_t && o ) noexcept
			:	m_thread{ std::exchange( o.m_thread, nullptr ) }
			,	m_factory{ std::exchange(
					o.m_factory, abstract_work_thread_factory_shptr_t{} )
				}
			{}

		work_thread_holder_t &
		operator=( work_thread_holder_t && o ) noexcept
			{
				work_thread_holder_t tmp{ std::move(o) };
				swap( *this, tmp );

				return *this;
			}

		~work_thread_holder_t() noexcept
			{
				if( m_thread )
					m_factory->release( *m_thread );
			}

		//FIXME: document this!
		[[nodiscard]]
		bool
		empty() const noexcept
		{
			return nullptr == m_thread;
		}

		//FIXME: document this!
		[[nodiscard]]
		explicit operator bool() const noexcept
		{
			return !empty();
		}

		//FIXME: document this!
		[[nodiscard]]
		bool operator!() const noexcept
		{
			return empty();
		}

		//FIXME: document this!
		[[nodiscard]]
		abstract_work_thread_t *
		unchecked_get() const noexcept
		{
			return m_thread;
		}
	};

//
// make_std_work_thread_factory
//
[[nodiscard]]
SO_5_FUNC
abstract_work_thread_factory_shptr_t
make_std_work_thread_factory();

} /* namespace so_5::disp */

