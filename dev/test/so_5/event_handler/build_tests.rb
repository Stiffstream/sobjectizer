#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/event_handler'

MxxRu::Cpp::composite_target {

	required_prj( "#{path}/subscribe_errors/prj.ut.rb" )
	required_prj( "#{path}/ignore_exception/prj.ut.rb" )
	required_prj( "#{path}/exception_reaction_inheritance/prj.ut.rb" )
	required_prj( "#{path}/unknown_exception_evt_handler/prj.ut.rb" )
	required_prj( "#{path}/unknown_exception_evt_start/prj.ut.rb" )
	required_prj( "#{path}/unknown_exception_evt_finish/prj.ut.rb" )
	required_prj( "#{path}/subscribe_before_reg/prj.ut.rb" )
	required_prj( "#{path}/drop_subscr_in_lambda_event_handler/prj.ut.rb" )
	required_prj( "#{path}/has_subscription_1/prj.ut.rb" )
	required_prj( "#{path}/has_subscription_2/prj.ut.rb" )
	required_prj( "#{path}/has_subscription_3/prj.ut.rb" )
	required_prj( "#{path}/has_subscription_4/prj.ut.rb" )
	required_prj( "#{path}/has_subscription_5/prj.ut.rb" )
	required_prj( "#{path}/has_subscription_6/prj.ut.rb" )
	required_prj( "#{path}/has_subscription_7/prj.ut.rb" )
	required_prj( "#{path}/has_subscription_8/prj.ut.rb" )
	required_prj( "#{path}/external_subscription/prj.ut.rb" )
	required_prj( "#{path}/deadletter_handler_simple/prj.ut.rb" )
	required_prj( "#{path}/deadletter_handler_unsubscribe/prj.ut.rb" )
	required_prj( "#{path}/deadletter_handler_unsubscribe_all_states/prj.ut.rb" )
	required_prj( "#{path}/deadletter_handler_has_handler/prj.ut.rb" )
	required_prj( "#{path}/deadletter_handler_formats/prj.ut.rb" )
	required_prj( "#{path}/deactivate_agent_simple/prj.ut.rb" )
	required_prj( "#{path}/deactivate_try_resubscribe/prj.ut.rb" )
	required_prj( "#{path}/deactivate_agent_evt_finish/prj.ut.rb" )
}

