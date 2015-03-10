require 'mxx_ru/binary_unittest'

path = 'test/so_5/message_limits/redirect_svc_too_deep/sc_mbox'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
