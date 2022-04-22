require 'mxx_ru/binary_unittest'

path = 'test/so_5/mbox/custom_direct_mbox_factory'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)

