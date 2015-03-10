require 'mxx_ru/binary_unittest'

path = 'test/so_5/message_limits/abort_app/sc_mbox'

MxxRu::setup_target(
	MxxRu::NegativeBinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
