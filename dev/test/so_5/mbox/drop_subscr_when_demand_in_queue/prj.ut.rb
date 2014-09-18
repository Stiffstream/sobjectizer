require 'mxx_ru/binary_unittest'

MxxRu::setup_target(
	MxxRu::Binary_unittest_target.new(
		"test/so_5/mbox/drop_subscr_when_demand_in_queue/prj.ut.rb",
		"test/so_5/mbox/drop_subscr_when_demand_in_queue/prj.rb" )
)
