require 'mxx_ru/binary_unittest'

path = 'test/so_5/coop/child_coop_map'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
