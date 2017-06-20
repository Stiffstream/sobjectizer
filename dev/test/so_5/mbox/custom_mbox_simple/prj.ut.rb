require 'mxx_ru/binary_unittest'

path = 'test/so_5/mbox/custom_mbox_simple'

MxxRu::setup_target(
	MxxRu::Binary_unittest_target.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
