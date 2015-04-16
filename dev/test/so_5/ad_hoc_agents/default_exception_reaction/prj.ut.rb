require 'mxx_ru/binary_unittest'

path = 'test/so_5/ad_hoc_agents/default_exception_reaction'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
