require 'mxx_ru/binary_unittest'

path = 'test/so_5/mbox/delivery_filters/simple_mpsc_immutable'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
