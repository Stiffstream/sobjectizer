require 'mxx_ru/binary_unittest'

path = 'test/so_5/event_handler/drop_all_subscriptions_simple'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
