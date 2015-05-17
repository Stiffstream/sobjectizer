require 'mxx_ru/binary_unittest'

path = 'test/so_5/mbox/delivery_filters/exception_in_filter'

MxxRu::setup_target(
	MxxRu::NegativeBinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
