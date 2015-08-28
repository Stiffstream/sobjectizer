#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/disp/prio_ot_strictly_ordered'

MxxRu::Cpp::composite_target {

	required_prj "#{path}/simple/prj.ut.rb"
	required_prj "#{path}/simple_seq1/prj.ut.rb"
	required_prj "#{path}/simple_seq2/prj.ut.rb"
	required_prj "#{path}/simple_seq3/prj.ut.rb"
	required_prj "#{path}/dereg_when_queue_not_empty/prj.ut.rb"
}
