require 'mxx_ru/binary_unittest'

path = 'test/so_5/details/invoke_noexcept_code'

MxxRu::setup_target(
	MxxRu::NegativeBinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
