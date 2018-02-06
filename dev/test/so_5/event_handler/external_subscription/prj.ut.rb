require 'mxx_ru/binary_unittest'

path = 'test/so_5/event_handler/external_subscription'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
