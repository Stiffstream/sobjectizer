require 'mxx_ru/binary_unittest'

path = 'test/so_5/mbox/unique_subscribers/repeated_subscribe'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
