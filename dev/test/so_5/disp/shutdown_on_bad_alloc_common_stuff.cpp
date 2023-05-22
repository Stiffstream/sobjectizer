namespace test
{


class a_child_t final : public so_5::agent_t
	{
	public:
		using so_5::agent_t::agent_t;
	};

class a_test_t final : public so_5::agent_t
	{
		struct msg_make_child final : public so_5::message_t
			{
				int m_number;

				msg_make_child( int number ) : m_number{ number }
					{}
			};

		struct msg_let_crash final : public so_5::signal_t {};

		so_5::disp_binder_shptr_t m_binder;

	public:
		a_test_t( context_t ctx, so_5::disp_binder_shptr_t binder )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_binder{ std::move(binder) }
			{}

		void
		so_define_agent() override
			{
				so_subscribe_self().event( &a_test_t::evt_make_child );
				so_subscribe_self().event( &a_test_t::evt_let_crash );
			}

		void
		so_evt_start() override
			{
				so_5::send< msg_make_child >( *this, 1 );
			}

	private:
		void
		evt_make_child( mhood_t< msg_make_child > cmd )
			{
				if( cmd->m_number < 100 )
					{
						so_5::introduce_child_coop(
								*this,
								m_binder,
								[]( so_5::coop_t & coop ) {
									coop.make_agent< a_child_t >();
								} );

						so_5::send< msg_make_child >( *this, cmd->m_number + 1 );
					}
				else
					{
						so_5::test::disp::custom_new::turn_should_throw_on();
						std::puts( "should_throw is turned on" );
						so_5::send< msg_let_crash >( *this );
					}
			}

		void
		evt_let_crash(mhood_t<msg_let_crash>)
			{
				// We shouldn't received this message!
				std::cerr << "evt_let_crash shouldn't be called!" << std::endl;
				std::abort();
			}
	};

} /* namespace test */

