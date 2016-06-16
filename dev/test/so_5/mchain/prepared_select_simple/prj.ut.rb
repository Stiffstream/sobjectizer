require 'mxx_ru/binary_unittest'

path = 'test/so_5/mchain/prepared_select_simple'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
