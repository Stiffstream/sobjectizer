require 'mxx_ru/binary_unittest'

path = 'test/so_5/mchain/limited_app_abort'

MxxRu::setup_target(
	MxxRu::NegativeBinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
