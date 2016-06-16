#!/usr/bin/ruby
gem 'Mxx_ru', '>= 1.6.11'
require 'mxx_ru/cpp'

require 'digest'

module So5

class ObjPlacement < MxxRu::Cpp::ObjPlacement
  USE_COMPILER_ID = true
  DO_NOT_USE_COMPILER_ID = false

  # Constructor
  def initialize( final_results_path, use_compiler_id = DO_NOT_USE_COMPILER_ID )
    @final_results_path = final_results_path
    @intermediate_path = File.join( @final_results_path, '_objs' )
    @use_compiler_id = use_compiler_id
  end

  # Make name for obj file.
  def get_obj( source_path_name, toolset, target )
    root_path = USE_COMPILER_ID == @use_compiler_id ?
        File.join( @intermediate_path, toolset.make_identification_string ) :
        @intermediate_path

    target_tmps = create_target_tmps_name( target )
    result = if source_path_name &&
        "" != source_path_name &&
        "." != source_path_name
      File.join( root_path, runtime_mode_path( target ), target_tmps,
        transform_path( source_path_name ) )
    else
      File.join( root_path, target_tmps, runtime_mode_path( target ) )
    end

    MxxRu::Util.ensure_path_exists( result )

    result
  end

  # Returns result of get_obj method.
  def get_mswin_res( source_path_name, toolset, target )
    get_obj( source_path_name, toolset, target )
  end

  # Returns final_results_path
  def get_lib( source_path_name, toolset, target )
    final_result_path_component( source_path_name, toolset, target )
  end

  # Returns final_results_path
  def get_dll( source_path_name, toolset, target )
    final_result_path_component( source_path_name, toolset, target )
  end

  # Returns final_results_path
  def get_exe( source_path_name, toolset, target )
    final_result_path_component( source_path_name, toolset, target )
  end

protected
  # Make final_results_path if needed and return name of it
  def final_result_path_component( target_root, toolset, target )
    root_path = USE_COMPILER_ID == @use_compiler_id ?
        File.join( @final_results_path, toolset.make_identification_string ) :
        @final_results_path

    result = if target_root &&
        "" != target_root &&
        "." != target_root
      File.join( root_path, runtime_mode_path( target ), target_root )
    else
      File.join( root_path, runtime_mode_path( target ) )
    end

    MxxRu::Util.ensure_path_exists( result )

    result
  end

  # Returns folder name, which is used for target's runtime mode.
  #
  # [_a_target_] Target, actions are performed for.
  def runtime_mode_path( a_target )
    case a_target.mxx_runtime_mode
      when MxxRu::Cpp::RUNTIME_DEBUG
        return 'debug'

      when MxxRu::Cpp::RUNTIME_RELEASE
        return 'release'

      else
        return 'default'
    end
  end

  def transform_path( path )
    normalized_value = path.gsub( /[\\\/\.:]/, '_' )
    if 24 < normalized_value.size
      r = normalized_value[ 0..3 ] + '__' + normalized_value[ -5..-1 ] + '-' +
        Digest::SHA1.new.update(normalized_value).hexdigest[ 0..11 ]
      normalized_value = r
    end
    normalized_value
  end

  def create_target_tmps_name( target )
    transform_path( target.prj_alias )
  end

end # class CustomSubdirObjPlacement

end # module So5

# vim:ts=2:sts=2:sw=2:expandtab

