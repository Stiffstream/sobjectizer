require 'mxx_ru/binary_unittest'

path = 'test/so_5/coop/destruction_order_1'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
