require 'mxx_ru/binary_unittest'

path = 'test/so_5/messages/make_transformed_message_holder'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
