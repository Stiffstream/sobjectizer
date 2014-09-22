require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target( MxxRu::BUILD_ROOT ) {

	toolset.force_cpp0x_std
	global_include_path "."

	if 'gcc' == toolset.name || 'clang' == toolset.name
		global_linker_option '-pthread'
	end

	# If there is local options file then use it.
	if FileTest.exist?( "local-build.rb" )
		required_prj "local-build.rb"
	else
		default_runtime_mode( MxxRu::Cpp::RUNTIME_RELEASE )
		MxxRu::enable_show_brief
	end

	required_prj( "so_5/prj.rb" )
}
