require 'mxx_ru/binary_unittest'

def setup_sample_as_unit_test(sample_name = nil)
  ut_name = File.basename( /^(.+):\d/.match( caller(1,1)[0] )[1] )
  sample_name = File.basename( ut_name, '.ut.rb' ) unless sample_name

  MxxRu::setup_target(
    MxxRu::BinaryUnittestTarget.new(
      "test/so_5/samples_as_unit_tests/#{ut_name}",
      "sample/so_5/#{sample_name}/prj.rb" )
  )
end
