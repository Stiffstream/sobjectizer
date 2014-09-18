require 'mxx_ru/binary_unittest'

Mxx_ru::setup_target(
	Mxx_ru::Binary_unittest_target.new(
		"test/so_5/svc/sync_request_and_wait_for/prj.ut.rb",
		"test/so_5/svc/sync_request_and_wait_for/prj.rb" )
)
