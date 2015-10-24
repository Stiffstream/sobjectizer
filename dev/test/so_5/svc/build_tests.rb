#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/svc'

	required_prj( "#{path}/stable_proxy/prj.ut.rb" )
	required_prj( "#{path}/simple_svc/prj.ut.rb" )
	required_prj( "#{path}/simple_lambda_svc/prj.ut.rb" )
	required_prj( "#{path}/simple_mutable_lambda_svc/prj.ut.rb" )
	required_prj( "#{path}/simple_svc_adhoc_agents/prj.ut.rb" )
	required_prj( "#{path}/svc_handler_exception/prj.ut.rb" )
	required_prj( "#{path}/no_svc_handlers/prj.ut.rb" )
	required_prj( "#{path}/several_svc_handlers/prj.ut.rb" )
	required_prj( "#{path}/svc_handler_not_called/prj.ut.rb" )
	required_prj( "#{path}/resending/prj.ut.rb" )
	required_prj( "#{path}/resending_sync_request/prj.ut.rb" )

	required_prj( "#{path}/make_sync_request/prj.ut.rb" )
	required_prj( "#{path}/sync_request_and_wait_for/prj.ut.rb" )

	required_prj( "#{path}/helper_functions/prj.ut.rb" )
}
