require 'mxx_ru/binary_unittest'

path = 'test/so_5/mbox/introduce_named_mbox/basic'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
