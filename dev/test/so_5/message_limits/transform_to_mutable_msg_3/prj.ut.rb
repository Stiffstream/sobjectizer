require 'mxx_ru/binary_unittest'

path = 'test/so_5/message_limits/transform_to_mutable_msg_3'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
