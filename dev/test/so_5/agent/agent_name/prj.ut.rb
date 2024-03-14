require 'mxx_ru/binary_unittest'

path = 'test/so_5/agent/agent_name'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
