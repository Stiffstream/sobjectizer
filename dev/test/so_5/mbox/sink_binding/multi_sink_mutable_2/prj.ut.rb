require 'mxx_ru/binary_unittest'

path = 'test/so_5/mbox/sink_binding/multi_sink_mutable_2'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)

