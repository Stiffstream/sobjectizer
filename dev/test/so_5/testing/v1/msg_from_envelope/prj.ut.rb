require 'mxx_ru/binary_unittest'

path = 'test/so_5/testing/v1/msg_from_envelope'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
