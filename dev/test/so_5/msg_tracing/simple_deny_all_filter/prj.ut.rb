require 'mxx_ru/binary_unittest'

path = 'test/so_5/msg_tracing/simple_deny_all_filter'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
