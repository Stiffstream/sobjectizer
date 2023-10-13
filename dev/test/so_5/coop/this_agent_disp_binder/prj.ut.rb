require 'mxx_ru/binary_unittest'

path = 'test/so_5/coop/this_agent_disp_binder'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
