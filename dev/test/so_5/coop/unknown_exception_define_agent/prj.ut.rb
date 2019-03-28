require 'mxx_ru/binary_unittest'

path = 'test/so_5/coop/unknown_exception_define_agent'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
