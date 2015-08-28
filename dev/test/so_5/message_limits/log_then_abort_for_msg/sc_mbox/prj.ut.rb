require 'mxx_ru/binary_unittest'

path = 'test/so_5/message_limits/log_then_abort_for_msg/sc_mbox'

MxxRu::setup_target(
	MxxRu::NegativeBinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
