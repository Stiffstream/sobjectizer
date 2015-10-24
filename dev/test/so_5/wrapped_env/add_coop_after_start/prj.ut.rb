require 'mxx_ru/binary_unittest'

path = 'test/so_5/wrapped_env/add_coop_after_start'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
