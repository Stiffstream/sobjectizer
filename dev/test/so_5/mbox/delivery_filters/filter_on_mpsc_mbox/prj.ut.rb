require 'mxx_ru/binary_unittest'

path = 'test/so_5/mbox/delivery_filters/filter_on_mpsc_mbox'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
