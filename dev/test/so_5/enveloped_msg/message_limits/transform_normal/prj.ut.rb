require 'mxx_ru/binary_unittest'

path = 'test/so_5/enveloped_msg/message_limits/transform_normal'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
