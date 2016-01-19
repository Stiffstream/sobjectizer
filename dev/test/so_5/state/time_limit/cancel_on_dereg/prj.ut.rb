require 'mxx_ru/binary_unittest'

path = 'test/so_5/state/time_limit/cancel_on_dereg'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
