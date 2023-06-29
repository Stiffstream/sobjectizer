/*
 * Implementation of a custom new/delete operators for testing purposes.
 */

#pragma once

#include <atomic>
#include <cstdlib>
#include <cstdio>

namespace so_5::test::disp::custom_new {

std::atomic< bool > g_should_throw_on_allocation{ false };

[[nodiscard]]
bool
should_throw() noexcept
	{
		return g_should_throw_on_allocation.load( std::memory_order_acquire );
	}

void
turn_should_throw_on() noexcept
	{
		g_should_throw_on_allocation = true;
	}

} /* namespace so_5::test::disp::custom_new */

//
// NOTE: the following code was borrowed from cppreference.com
// https://en.cppreference.com/w/cpp/memory/new/operator_new
//

// no inline, required by [replacement.functions]/3
void* operator new(std::size_t sz)
	{
		if( so_5::test::disp::custom_new::should_throw() )
			throw std::bad_alloc{};

		if (sz == 0)
			++sz; // avoid std::malloc(0) which may return nullptr on success

		if (void *ptr = std::malloc(sz))
			return ptr;

		throw std::bad_alloc{}; // required by [new.delete.single]/3
	}

// no inline, required by [replacement.functions]/3
void* operator new[](std::size_t sz)
	{
		if( so_5::test::disp::custom_new::should_throw() )
			throw std::bad_alloc{};

		if (sz == 0)
			++sz; // avoid std::malloc(0) which may return nullptr on success

		if (void *ptr = std::malloc(sz))
			return ptr;

		throw std::bad_alloc{}; // required by [new.delete.single]/3
	}

void operator delete(void* ptr) noexcept
	{
		std::free(ptr);
	}

void operator delete(void* ptr, std::size_t /*size*/) noexcept
	{
		std::free(ptr);
	}

void operator delete[](void* ptr) noexcept
	{
		std::free(ptr);
	}

void operator delete[](void* ptr, std::size_t /*size*/) noexcept
	{
		std::free(ptr);
	}

