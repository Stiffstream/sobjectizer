require 'mxx_ru/binary_unittest'

path = 'test/so_5/messages/tuple_as_message'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
