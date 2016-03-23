require 'mxx_ru/binary_unittest'

path = 'test/so_5/mchain/select_mthread_read'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
