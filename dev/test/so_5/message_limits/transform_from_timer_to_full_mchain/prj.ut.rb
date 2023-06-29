require 'mxx_ru/binary_unittest'

path = 'test/so_5/message_limits/transform_from_timer_to_full_mchain'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
