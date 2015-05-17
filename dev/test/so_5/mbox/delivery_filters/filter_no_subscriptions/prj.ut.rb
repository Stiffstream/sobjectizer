require 'mxx_ru/binary_unittest'

path = 'test/so_5/mbox/delivery_filters/filter_no_subscriptions'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
