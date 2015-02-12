/*
 * This test is inspired by advice of Dmitry Vyukov and is based on
 * test implementation from LLVM code base:
 * http://llvm.org/viewvc/llvm-project/compiler-rt/trunk/lib/tsan/unit_tests/tsan_mutex_test.cc?view=markup&pathrev=164021
 */
#include <cstdint>
#include <thread>
#include <mutex>

#include <utest_helper_1/h/helper.hpp>

#include <so_5/h/spinlocks.hpp>

template< typename M, typename WRITE_LOCK, typename READ_LOCK >
class TestData
{
	public:
		explicit TestData( M & m )
			: mtx_(m)
		{
			for (int i = 0; i < kSize; i++)
			data_[i] = 0;
		}

		void Write()
		{
			WRITE_LOCK l(mtx_);
			T v0 = data_[0];
			for (int i = 0; i < kSize; i++) {
				UT_CHECK_EQ(data_[i], v0);
				data_[i]++;
			}
		}

		void Read()
		{
			READ_LOCK l(mtx_);
			T v0 = data_[0];
			for (int i = 0; i < kSize; i++) {
				UT_CHECK_EQ(data_[i], v0);
			}
		}

		void Backoff() {
			volatile T data[kSize] = {};
			for (int i = 0; i < kSize; i++) {
				data[i]++;
				UT_CHECK_EQ(data[i], 1);
			}
		}

	private:
		static const int kCacheLineSize = 64;
		static const int kSize = 64;
		typedef std::int64_t T;
		M & mtx_;
		char pad_[kCacheLineSize];
		T data_[kSize];
};

const int kThreads = 8;
const int kWriteRate = 1024;
const int kIters = 64*1024;

template< typename T >
void write_mutex_thread( T * data ) {
  for (int i = 0; i < kIters; i++) {
    data->Write();
    data->Backoff();
  }
}

template< typename T >
void read_mutex_thread( T * data ) {
  for (int i = 0; i < kIters; i++) {
    if ((i % kWriteRate) == 0)
      data->Write();
    else
      data->Read();
    data->Backoff();
  }
}

template< typename THREAD_FUNC, typename THREAD_ARG >
void
run_test_threads( THREAD_FUNC func, THREAD_ARG * arg )
{
	std::thread threads[kThreads];
	for( int i = 0; i != kThreads; ++i )
		threads[i] = std::thread( func, arg );
	for( int i = 0; i != kThreads; ++i )
		threads[i].join();
}

UT_UNIT_TEST(Spinlock_Write) {
	so_5::default_spinlock_t lock;
	TestData< so_5::default_spinlock_t,
			std::lock_guard< so_5::default_spinlock_t >,
			std::lock_guard< so_5::default_spinlock_t > > data( lock );

	run_test_threads( write_mutex_thread< decltype(data) >, &data );
}

UT_UNIT_TEST(RWSpinlock_ReadWrite) {
	so_5::default_rw_spinlock_t lock;
	TestData< so_5::default_rw_spinlock_t,
			std::lock_guard< so_5::default_rw_spinlock_t >,
			so_5::read_lock_guard_t< so_5::default_rw_spinlock_t > > data( lock );

	run_test_threads( read_mutex_thread< decltype(data) >, &data );
}

int main()
{
	UT_RUN_UNIT_TEST( Spinlock_Write )
	UT_RUN_UNIT_TEST( RWSpinlock_ReadWrite )
}

