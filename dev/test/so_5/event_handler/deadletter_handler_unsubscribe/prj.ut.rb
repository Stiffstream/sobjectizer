require 'mxx_ru/binary_unittest'

path = 'test/so_5/event_handler/deadletter_handler_unsubscribe'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
