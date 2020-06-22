#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/coop'

	required_prj( "#{path}/dereg_empty_coop/prj.ut.rb" )
	required_prj( "#{path}/reg_some_and_stop_1/prj.ut.rb" )
	required_prj( "#{path}/reg_some_and_stop_2/prj.ut.rb" )
	required_prj( "#{path}/reg_some_and_stop_3/prj.ut.rb" )
	required_prj( "#{path}/throw_on_define_agent/prj.ut.rb" )
	required_prj( "#{path}/unknown_exception_define_agent/prj.ut.rb" )
	required_prj( "#{path}/throw_on_bind_to_disp/prj.ut.rb" )
	required_prj( "#{path}/throw_on_bind_to_disp_2/prj.ut.rb" )
	required_prj( "#{path}/coop_notify_1/prj.ut.rb" )
	required_prj( "#{path}/coop_notify_2/prj.ut.rb" )
	required_prj( "#{path}/coop_notify_3/prj.ut.rb" )
	required_prj( "#{path}/parent_child_1/prj.ut.rb" )
	required_prj( "#{path}/parent_child_2/prj.ut.rb" )
	required_prj( "#{path}/parent_child_3/prj.ut.rb" )
	required_prj( "#{path}/parent_child_4/prj.ut.rb" )
	required_prj( "#{path}/user_resource/prj.ut.rb" )
	required_prj( "#{path}/introduce_coop/prj.ut.rb" )
	required_prj( "#{path}/introduce_coop_2/prj.ut.rb" )
	required_prj( "#{path}/introduce_coop_3/prj.ut.rb" )
	required_prj( "#{path}/introduce_coop_4/prj.ut.rb" )
	required_prj( "#{path}/create_child_coop_5_5_8/prj.ut.rb" )
	required_prj( "#{path}/shutdown_while_reg/prj.ut.rb" )
	required_prj( "#{path}/child_coop_map/prj.ut.rb" )
}
