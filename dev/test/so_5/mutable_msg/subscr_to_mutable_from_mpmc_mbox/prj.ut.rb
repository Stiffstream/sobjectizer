require 'mxx_ru/binary_unittest'

path = 'test/so_5/mutable_msg/subscr_to_mutable_from_mpmc_mbox'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
