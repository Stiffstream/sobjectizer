/*!
	Вспомогательная функциональность для тестирования.
	\file utest_helper_1/h/helper.hpp
*/
#if !defined( UTEST_HELPER_1_HELPER_HELPER_HPP )
#define UTEST_HELPER_1_HELPER_HELPER_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <stack>

namespace utest_helper_1
{

//! Исключение выбрасываемое утверждениями.
class check_ex_t : public std::runtime_error
{
public:
	check_ex_t(
		const std::string & err )
		:	std::runtime_error( err )
	{}
};

#define UT_CHECK_THROW_IMPL( file, line, exception_class, body )\
do \
{ \
	std::ostringstream err_stream; \
	err_stream << file << "(" << line << "): "; \
	bool exception_thrown_impl_detail_ = false; \
	try { \
		body; \
	} \
	catch( const exception_class & ) { \
		exception_thrown_impl_detail_ = true; \
	} \
	catch( const std::exception & x ) { \
		err_stream << "unit test error: check throw failed: expected " \
			<< #exception_class " but caught: std::exception: " << x.what() << "\n"; \
	} \
	catch( ... ) { \
		err_stream << "unit test error: check throw failed: expected " \
			<< #exception_class << " but caught unknown exception\n"; \
	} \
	if( !exception_thrown_impl_detail_ ) \
	{ \
		throw ::utest_helper_1::check_ex_t( err_stream.str() ); \
	} \
} \
while( false )

/*!
	Проверка на выбрасывание исключения.
	\code
		UT_CHECK_THROW( connection_ex_t, m_db.open( connection_params ) );
	\endcode

*/
#define UT_CHECK_THROW( exception_class, body ) \
UT_CHECK_THROW_IMPL(__FILE__, __LINE__, exception_class, body )


/*!
	Функция для проверки условия.
*/
inline void
check_and_throw(
	//! Результат проверки условия.
	bool check_result,
	//! Условие.
	const std::string & condition,
	//! Файл.
	const std::string & file,
	//! Номер строки.
	int line )
{
	if( !check_result )
	{
		std::ostringstream err_stream;
		err_stream
			<< file << "("
			<< line << "): "
			"unit test error: check condition failed: '" << condition << "'\n";
		throw check_ex_t( err_stream.str() );
	}
}

#define UT_CHECK_CONDITION( condition ) \
	::utest_helper_1::check_and_throw( (condition),  #condition, __FILE__, __LINE__ )

//! Вспомогательная функция для формированияописания ошибки
//! в случае если сравнение не прошло проверку.
template <class LEFT, class RIGHT >
inline void
cmp_failed(
	const std::string & cmp_str,
	const LEFT & left,
	const RIGHT & right,
	const std::string & left_str,
	const std::string & right_str,
	const std::string & file,
	int line )
{
	std::ostringstream err_stream;
	err_stream
		<< file << "("
		<< line << "): "
		"unit test error: comparison failed: '"
		<< left_str << cmp_str << right_str << "' where\n"
		"  [\n"
		"    " << left_str << " is " << left << ",\n"
		"    " << right_str << " is " << right << "\n"
		"  ]\n";
	throw check_ex_t( err_stream.str() );

}

//! Функция для проверки на равенство.
template <class LEFT, class RIGHT >
inline void
check_eq_and_throw(
	const LEFT & left,
	const RIGHT & right,
	const std::string & left_str,
	const std::string & right_str,
	const std::string & file,
	int line )
{
	if( !( left == right) )
	{
		cmp_failed( " == ", left, right, left_str, right_str, file, line );
	}
}

#define UT_CHECK_EQ( left, right ) \
	::utest_helper_1::check_eq_and_throw(\
		(left), (right), #left, #right, __FILE__, __LINE__ )

//! Функция для проверки на неравенство.
template <class LEFT, class RIGHT >
inline void
check_ne_and_throw(
	const LEFT & left,
	const RIGHT & right,
	const std::string & left_str,
	const std::string & right_str,
	const std::string & file,
	int line )
{
	if( !( left != right) )
	{
		cmp_failed( " != ", left, right, left_str, right_str, file, line );
	}
}

#define UT_CHECK_NE( left, right ) \
	::utest_helper_1::check_ne_and_throw(\
		(left), (right), #left, #right, __FILE__, __LINE__ )

//! Функция для проверки на >=.
template <class LEFT, class RIGHT >
inline void
check_ge_and_throw(
	const LEFT & left,
	const RIGHT & right,
	const std::string & left_str,
	const std::string & right_str,
	const std::string & file,
	int line )
{
	if( !( left >= right) )
	{
		cmp_failed( " >= ", left, right, left_str, right_str, file, line );
	}
}

#define UT_CHECK_GE( left, right ) \
	::utest_helper_1::check_ge_and_throw(\
		(left), (right), #left, #right, __FILE__, __LINE__ )

//! Функция для проверки на >.
template <class LEFT, class RIGHT >
inline void
check_gt_and_throw(
	const LEFT & left,
	const RIGHT & right,
	const std::string & left_str,
	const std::string & right_str,
	const std::string & file,
	int line )
{
	if( !( left > right) )
	{
		cmp_failed( " > ", left, right, left_str, right_str, file, line );
	}
}

#define UT_CHECK_GT( left, right ) \
	::utest_helper_1::check_gt_and_throw(\
		(left), (right), #left, #right, __FILE__, __LINE__ )

//!  Функция для проверки на <=.
template <class LEFT, class RIGHT >
inline void
check_le_and_throw(
	const LEFT & left,
	const RIGHT & right,
	const std::string & left_str,
	const std::string & right_str,
	const std::string & file,
	int line )
{
	if( !( left <= right) )
	{
		cmp_failed( " <= ", left, right, left_str, right_str, file, line );
	}
}

#define UT_CHECK_LE( left, right ) \
	::utest_helper_1::check_le_and_throw(\
		(left), (right), #left, #right, __FILE__, __LINE__ )

//!  Функция для проверки на <.
template <class LEFT, class RIGHT >
inline void
check_lt_and_throw(
	const LEFT & left,
	const RIGHT & right,
	const std::string & left_str,
	const std::string & right_str,
	const std::string & file,
	int line )
{
	if( !( left < right) )
	{
		cmp_failed( " < ", left, right, left_str, right_str, file, line );
	}
}

#define UT_CHECK_LT( left, right ) \
	::utest_helper_1::check_lt_and_throw(\
		(left), (right), #left, #right, __FILE__, __LINE__ )

typedef std::stack< std::string > context_stack_t;

//! Вспомогательный класс вывода контекста.
class context_streamer_t
{
	public:
		inline context_streamer_t(
			context_stack_t & context_stack,
			const std::string & name,
			const std::string & file,
			unsigned int line )
			:
				m_context_stack( context_stack )
		{
			m_sout << "---- ---- ---- ---- " << name << "\n";
			m_header = m_sout.str();
			m_sout.str( "" );
			m_sout << "file: " << file << "\n"
				"line: " << line << "\n";
			m_footer = m_sout.str();
			m_sout.str( "" );
		}

		inline ~context_streamer_t()
		{
			std::string final = m_header;
			const std::string extra = m_sout.str();
			if( !extra.empty() )
				final += "info: " + extra + "\n";

			final += m_footer + "\n";

			m_context_stack.push( final );
		}

		inline std::ostream &
		query_stream() const
		{
			return m_sout;
		}

	private:
		std::string m_header;
		std::string m_footer;
		context_stack_t & m_context_stack;
		mutable std::ostringstream m_sout;
};

//! A macro for simplification of unit-test definition.
#define UT_UNIT_TEST( test_name ) \
	struct test_name ## _internal_unique_name_class_ { \
		::utest_helper_1::context_stack_t test_context_impl_detail_; \
		void run_test(); \
	}; \
	int test_name() \
	{ \
		test_name ## _internal_unique_name_class_ instance; \
		bool has_exceptions = false; \
		try{ \
			instance.run_test(); \
			std::cout << "Unit test: " #test_name " OK" << std::endl;\
		} catch( const ::utest_helper_1::check_ex_t & ex ) { \
			std::cerr << ex.what() \
				<< "Unit test '" << #test_name << "' failed\n"; \
			has_exceptions = true; \
		} catch( const ::std::exception & ex ) { \
			std::cerr << "Unit test '" << #test_name << "' failed\n" \
			"Exception: " << ex.what(); \
			has_exceptions = true; \
		} catch( ... ) { \
			std::cerr << "\n\nUnit test '" << #test_name << "' failed\n" \
			"Unknown exception"; \
			has_exceptions = true; \
		} \
		\
		if( has_exceptions && !instance.test_context_impl_detail_.empty() ) \
		{ \
			std::cerr << "\nContext:\n"; \
			while( !instance.test_context_impl_detail_.empty() ) \
			{ \
				std::cerr << instance.test_context_impl_detail_.top();\
				instance.test_context_impl_detail_.pop(); \
			} \
		} \
		return has_exceptions ? -1 : 0; \
	} \
	void test_name ## _internal_unique_name_class_::run_test()

//! Добавить еденицу описания контекста.
#define UT_PUSH_CONTEXT( name ) \
	::utest_helper_1::context_streamer_t( \
		test_context_impl_detail_, \
		name, \
		__FILE__, \
		__LINE__ ).query_stream()

//! Извлечь еденицу описания из контекста.
#define UT_POP_CONTEXT() \
		test_context_impl_detail_.pop()

//! Для передачи контекста в другие функции.
#define UT_CONTEXT_DECL ::utest_helper_1::context_stack_t & test_context_impl_detail_

//! Передать вызываемой функции контекст
#define UT_CONTEXT test_context_impl_detail_

//! Вспомогательный макрос для вызова юнит теста из функции main.
#define UT_RUN_UNIT_TEST( test_name ) \
	if( 0 != test_name() ) \
	{\
		return 1; \
	}

} /* namespace utest_helper_1 */

#endif
