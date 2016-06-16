require 'mxx_ru/binary_unittest'

def setup_sample_as_unit_test(sample_name = nil)
  ut_name = File.basename( /^(.+):\d/.match( caller(1)[0] )[1], '.ut.rb' )
  sample_name = ut_name unless sample_name
  prj_name = 'prj'

  if ut_name.end_with?( '-static' )
    if sample_name == ut_name
    	sample_name = ut_name[ 0..-8 ]
    end
    prj_name << '_s'
  end

  MxxRu::setup_target(
    MxxRu::BinaryUnittestTarget.new(
      "test/so_5/samples_as_unit_tests/#{ut_name}.ut.rb",
      "sample/so_5/#{sample_name}/#{prj_name}.rb" )
  )
end
