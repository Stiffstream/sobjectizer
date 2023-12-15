#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/mbox/sink_binding'

MxxRu::Cpp::composite_target {

  required_prj( "#{path}/simple/prj.ut.rb" )
  required_prj( "#{path}/simple_2/prj.ut.rb" )
  required_prj( "#{path}/simple_with_delivery_filter/prj.ut.rb" )
  required_prj( "#{path}/simple_with_delivery_filter_2/prj.ut.rb" )
  required_prj( "#{path}/single_sink_clear/prj.ut.rb" )
  required_prj( "#{path}/single_sink_too_deep/prj.ut.rb" )
  required_prj( "#{path}/single_sink_mutable/prj.ut.rb" )
  required_prj( "#{path}/single_sink_mutable_with_dr/prj.ut.rb" )

  required_prj( "#{path}/multi_sink_simple/prj.ut.rb" )
  required_prj( "#{path}/multi_sink_simple_2/prj.ut.rb" )
  required_prj( "#{path}/multi_sink_with_delivery_filter/prj.ut.rb" )
  required_prj( "#{path}/multi_sink_with_delivery_filter_2/prj.ut.rb" )
  required_prj( "#{path}/multi_sink_unbind/prj.ut.rb" )
  required_prj( "#{path}/multi_sink_mutable/prj.ut.rb" )
  required_prj( "#{path}/multi_sink_mutable_2/prj.ut.rb" )
  required_prj( "#{path}/multi_sink_mutable_with_dr/prj.ut.rb" )

  required_prj( "#{path}/bind_then_transform_msg/prj.ut.rb" )
  required_prj( "#{path}/bind_then_transform_signal/prj.ut.rb" )
  required_prj( "#{path}/transformer_redirect_deep/prj.ut.rb" )
  required_prj( "#{path}/transformer_and_envelope/prj.ut.rb" )
}
