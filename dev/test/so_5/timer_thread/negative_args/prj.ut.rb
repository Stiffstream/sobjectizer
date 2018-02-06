require 'mxx_ru/binary_unittest'

path = 'test/so_5/timer_thread/negative_args'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
