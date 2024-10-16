require 'mxx_ru/binary_unittest'

path = 'test/so_5/details/null_mutex_lock_shared'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
