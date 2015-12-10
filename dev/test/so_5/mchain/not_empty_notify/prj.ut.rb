require 'mxx_ru/binary_unittest'

path = 'test/so_5/mchain/not_empty_notify'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
