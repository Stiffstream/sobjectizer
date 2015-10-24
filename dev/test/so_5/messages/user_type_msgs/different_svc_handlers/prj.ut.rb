require 'mxx_ru/binary_unittest'

path = 'test/so_5/messages/user_type_msgs/different_svc_handlers'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
