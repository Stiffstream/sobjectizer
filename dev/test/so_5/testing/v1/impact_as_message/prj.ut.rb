require 'mxx_ru/binary_unittest'

path = 'test/so_5/testing/v1/impact_as_message'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
