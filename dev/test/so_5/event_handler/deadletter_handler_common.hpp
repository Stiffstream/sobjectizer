#pragma once

#include <so_5/all.hpp>

class direct_mbox_case_t
{
	const so_5::agent_t & m_owner;
public :
	direct_mbox_case_t(
		const so_5::agent_t & owner )
		:	m_owner(owner)
	{}

	const so_5::mbox_t &
	mbox() const SO_5_NOEXCEPT { return m_owner.so_direct_mbox(); }
};

class mpmc_mbox_case_t
{
	const so_5::mbox_t m_mbox;
public:
	mpmc_mbox_case_t(
		const so_5::agent_t & owner )
		:	m_mbox( owner.so_environment().create_mbox() )
	{}

	const so_5::mbox_t &
	mbox() const SO_5_NOEXCEPT { return m_mbox; }
};

template<
	typename Mbox_Case,
	typename Msg_Type,
	template <class, class> class Test_Agent >
void
introduce_test_agent( so_5::environment_t & env )
{
	env.introduce_coop( [&]( so_5::coop_t & coop ) {
			coop.make_agent< Test_Agent<Mbox_Case, Msg_Type> >();
		} );
}

