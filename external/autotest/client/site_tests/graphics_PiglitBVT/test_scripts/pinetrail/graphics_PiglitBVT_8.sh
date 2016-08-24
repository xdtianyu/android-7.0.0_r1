#!/bin/bash


need_pass=71
failures=0
PIGLIT_PATH=/usr/local/piglit/lib/piglit/
export PIGLIT_SOURCE_DIR=/usr/local/piglit/lib/piglit/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PIGLIT_PATH/lib
export DISPLAY=:0
export XAUTHORITY=/home/chronos/.Xauthority


function run_test()
{
  local name="$1"
  local time="$2"
  local command="$3"
  echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
  echo "+ Running test [$name] of expected runtime $time sec: [$command]"
  sync
  $command
  if [ $? == 0 ] ; then
    let "need_pass--"
    echo "+ pass :: $name"
  else
    let "failures++"
    echo "+ fail :: $name"
  fi
}


pushd $PIGLIT_PATH
run_test "spec/ARB_vertex_program/vp-arl-constant-array-varying" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-arl-constant-array-varying.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-arl-env-array" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-arl-env-array.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-arl-local-array" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-arl-local-array.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-arl-neg-array" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-arl-neg-array.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-arl-neg-array-2" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-arl-neg-array-2.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-bad-program" 0.0 "bin/vp-bad-program -auto"
run_test "spec/ARB_vertex_program/vp-constant-array" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-constant-array.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-constant-array-huge" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-constant-array-huge.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-constant-negate" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-constant-negate.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-exp-alias" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-exp-alias.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-max" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-max.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-max-array" 0.0 "bin/vp-max-array -auto"
run_test "spec/ARB_vertex_program/vp-min" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-min.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-sge-alias" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-sge-alias.vpfp -fbo -auto"
run_test "spec/ARB_vertex_program/vp-two-constants" 0.0 "bin/vpfp-generic tests/shaders/generic/vp-two-constants.vpfp -fbo -auto"
run_test "spec/ARB_vertex_type_10f_11f_11f_rev/arb_vertex_type_10f_11f_11f_rev-api-errors" 0.0 "bin/arb_vertex_type_10f_11f_11f_rev-api-errors -auto"
run_test "spec/ARB_vertex_type_10f_11f_11f_rev/arb_vertex_type_10f_11f_11f_rev-draw-vertices" 0.0 "bin/arb_vertex_type_10f_11f_11f_rev-draw-vertices -fbo -auto"
run_test "spec/ARB_vertex_type_2_10_10_10_rev/attribs" 0.0 "bin/attribs GL_ARB_vertex_type_2_10_10_10_rev -fbo -auto"
run_test "spec/ATI_texture_compression_3dc/invalid formats" 0.0 "bin/arb_texture_compression-invalid-formats 3dc -fbo -auto"
run_test "spec/EXT_fog_coord/ext_fog_coord-modes" 0.0 "bin/ext_fog_coord-modes -auto"
run_test "spec/EXT_framebuffer_blit/fbo-blit" 0.0 "bin/fbo-blit -auto"
run_test "spec/EXT_framebuffer_blit/fbo-copypix" 0.0 "bin/fbo-copypix -auto"
run_test "spec/EXT_framebuffer_blit/fbo-readdrawpix" 0.0 "bin/fbo-readdrawpix -auto"
run_test "spec/EXT_framebuffer_blit/fbo-sys-blit" 0.0 "bin/fbo-sys-blit -auto"
run_test "spec/EXT_framebuffer_blit/fbo-sys-sub-blit" 0.0 "bin/fbo-sys-sub-blit -auto"
run_test "spec/EXT_framebuffer_object/fbo-1d" 0.0 "bin/fbo-1d -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-3d" 0.0 "bin/fbo-3d -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-alphatest-nocolor" 0.0 "bin/fbo-alphatest-nocolor -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-alphatest-nocolor-ff" 0.0 "bin/fbo-alphatest-nocolor-ff -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-bind-renderbuffer" 0.0 "bin/fbo-bind-renderbuffer -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-clear-formats" 0.0 "bin/fbo-clear-formats -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-clearmipmap" 0.0 "bin/fbo-clearmipmap -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-copyteximage" 0.0 "bin/fbo-copyteximage -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-copyteximage-simple" 0.0 "bin/fbo-copyteximage-simple -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-cubemap" 0.0 "bin/fbo-cubemap -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-depthtex" 0.0 "bin/fbo-depthtex -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-finish-deleted" 0.0 "bin/fbo-finish-deleted -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-flushing" 0.0 "bin/fbo-flushing -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-flushing-2" 0.0 "bin/fbo-flushing-2 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-generatemipmap" 0.0 "bin/fbo-generatemipmap -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-generatemipmap-noimage" 0.0 "bin/fbo-generatemipmap-noimage -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-generatemipmap-nonsquare" 0.0 "bin/fbo-generatemipmap-nonsquare -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-generatemipmap-npot" 0.0 "bin/fbo-generatemipmap-npot -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-generatemipmap-scissor" 0.0 "bin/fbo-generatemipmap-scissor -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-generatemipmap-viewport" 0.0 "bin/fbo-generatemipmap-viewport -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-maxsize" 0.0 "bin/fbo-maxsize -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-nodepth-test" 0.0 "bin/fbo-nodepth-test -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-nostencil-test" 0.0 "bin/fbo-nostencil-test -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-readpixels" 0.0 "bin/fbo-readpixels -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-readpixels-depth-formats" 0.0 "bin/fbo-readpixels-depth-formats -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-scissor-bitmap" 0.0 "bin/fbo-scissor-bitmap -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX1-clear" 0.0 "bin/fbo-stencil clear GL_STENCIL_INDEX1 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX1-copypixels" 0.0 "bin/fbo-stencil copypixels GL_STENCIL_INDEX1 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX1-drawpixels" 0.0 "bin/fbo-stencil drawpixels GL_STENCIL_INDEX1 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX1-readpixels" 0.0 "bin/fbo-stencil readpixels GL_STENCIL_INDEX1 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX16-clear" 0.0 "bin/fbo-stencil clear GL_STENCIL_INDEX16 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX16-copypixels" 0.0 "bin/fbo-stencil copypixels GL_STENCIL_INDEX16 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX16-drawpixels" 0.0 "bin/fbo-stencil drawpixels GL_STENCIL_INDEX16 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX16-readpixels" 0.0 "bin/fbo-stencil readpixels GL_STENCIL_INDEX16 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX4-clear" 0.0 "bin/fbo-stencil clear GL_STENCIL_INDEX4 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX4-copypixels" 0.0 "bin/fbo-stencil copypixels GL_STENCIL_INDEX4 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX4-drawpixels" 0.0 "bin/fbo-stencil drawpixels GL_STENCIL_INDEX4 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX4-readpixels" 0.0 "bin/fbo-stencil readpixels GL_STENCIL_INDEX4 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX8-clear" 0.0 "bin/fbo-stencil clear GL_STENCIL_INDEX8 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX8-copypixels" 0.0 "bin/fbo-stencil copypixels GL_STENCIL_INDEX8 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX8-drawpixels" 0.0 "bin/fbo-stencil drawpixels GL_STENCIL_INDEX8 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-stencil-GL_STENCIL_INDEX8-readpixels" 0.0 "bin/fbo-stencil readpixels GL_STENCIL_INDEX8 -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-storage-completeness" 0.0 "bin/fbo-storage-completeness -fbo -auto"
run_test "spec/EXT_framebuffer_object/fbo-storage-formats" 0.0 "bin/fbo-storage-formats -fbo -auto"
run_test "spec/EXT_framebuffer_object/fdo20701" 0.0 "bin/fdo20701 -fbo -auto"
run_test "spec/EXT_packed_depth_stencil/errors" 0.0 "bin/ext_packed_depth_stencil-errors -fbo -auto"
popd

if [ $need_pass == 0 ] ; then
  echo "+---------------------------------------------+"
  echo "| Overall pass, as all 71 tests have passed. |"
  echo "+---------------------------------------------+"
else
  echo "+-----------------------------------------------------------+"
  echo "| Overall failure, as $need_pass tests did not pass and $failures failed. |"
  echo "+-----------------------------------------------------------+"
fi
exit $need_pass

