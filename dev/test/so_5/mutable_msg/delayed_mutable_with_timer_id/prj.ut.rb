require 'mxx_ru/binary_unittest'

path = 'test/so_5/mutable_msg/delayed_mutable_with_timer_id'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
