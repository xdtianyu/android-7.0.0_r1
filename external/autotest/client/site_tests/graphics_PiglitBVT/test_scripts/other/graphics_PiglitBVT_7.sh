#!/bin/bash


need_pass=86
failures=0
PIGLIT_PATH=/usr/local/piglit/lib64/piglit/
export PIGLIT_SOURCE_DIR=/usr/local/piglit/lib64/piglit/
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
run_test "spec/EXT_timer_query/time-elapsed" 0.0 "bin/ext_timer_query-time-elapsed -fbo -auto"
run_test "spec/EXT_timer_query/timer_query" 0.0 "bin/timer_query -auto"
run_test "spec/EXT_transform_feedback/alignment 0" 0.0 "bin/ext_transform_feedback-alignment 0 -fbo -auto"
run_test "spec/EXT_transform_feedback/alignment 12" 0.0 "bin/ext_transform_feedback-alignment 12 -fbo -auto"
run_test "spec/EXT_transform_feedback/alignment 4" 0.0 "bin/ext_transform_feedback-alignment 4 -fbo -auto"
run_test "spec/EXT_transform_feedback/alignment 8" 0.0 "bin/ext_transform_feedback-alignment 8 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors begin_active" 0.0 "bin/ext_transform_feedback-api-errors begin_active -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_base_active" 0.0 "bin/ext_transform_feedback-api-errors bind_base_active -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_base_max" 0.0 "bin/ext_transform_feedback-api-errors bind_base_max -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_offset_active" 0.0 "bin/ext_transform_feedback-api-errors bind_offset_active -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_offset_max" 0.0 "bin/ext_transform_feedback-api-errors bind_offset_max -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_offset_offset_1" 0.0 "bin/ext_transform_feedback-api-errors bind_offset_offset_1 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_offset_offset_2" 0.0 "bin/ext_transform_feedback-api-errors bind_offset_offset_2 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_offset_offset_3" 0.0 "bin/ext_transform_feedback-api-errors bind_offset_offset_3 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_offset_offset_5" 0.0 "bin/ext_transform_feedback-api-errors bind_offset_offset_5 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_pipeline" 0.0 "bin/ext_transform_feedback-api-errors bind_pipeline -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_active" 0.0 "bin/ext_transform_feedback-api-errors bind_range_active -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_max" 0.0 "bin/ext_transform_feedback-api-errors bind_range_max -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_offset_1" 0.0 "bin/ext_transform_feedback-api-errors bind_range_offset_1 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_offset_2" 0.0 "bin/ext_transform_feedback-api-errors bind_range_offset_2 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_offset_3" 0.0 "bin/ext_transform_feedback-api-errors bind_range_offset_3 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_offset_5" 0.0 "bin/ext_transform_feedback-api-errors bind_range_offset_5 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_size_0" 0.0 "bin/ext_transform_feedback-api-errors bind_range_size_0 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_size_1" 0.0 "bin/ext_transform_feedback-api-errors bind_range_size_1 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_size_2" 0.0 "bin/ext_transform_feedback-api-errors bind_range_size_2 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_size_3" 0.0 "bin/ext_transform_feedback-api-errors bind_range_size_3 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_size_5" 0.0 "bin/ext_transform_feedback-api-errors bind_range_size_5 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors bind_range_size_m4" 0.0 "bin/ext_transform_feedback-api-errors bind_range_size_m4 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors end_inactive" 0.0 "bin/ext_transform_feedback-api-errors end_inactive -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors interleaved_no_varyings" 0.0 "bin/ext_transform_feedback-api-errors interleaved_no_varyings -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors interleaved_ok_base" 0.0 "bin/ext_transform_feedback-api-errors interleaved_ok_base -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors interleaved_ok_offset" 0.0 "bin/ext_transform_feedback-api-errors interleaved_ok_offset -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors interleaved_ok_range" 0.0 "bin/ext_transform_feedback-api-errors interleaved_ok_range -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors interleaved_unbound" 0.0 "bin/ext_transform_feedback-api-errors interleaved_unbound -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors link_current_active" 0.0 "bin/ext_transform_feedback-api-errors link_current_active -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors link_other_active" 0.0 "bin/ext_transform_feedback-api-errors link_other_active -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors no_prog_active" 0.0 "bin/ext_transform_feedback-api-errors no_prog_active -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors not_a_program" 0.0 "bin/ext_transform_feedback-api-errors not_a_program -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors separate_no_varyings" 0.0 "bin/ext_transform_feedback-api-errors separate_no_varyings -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors separate_ok_1" 0.0 "bin/ext_transform_feedback-api-errors separate_ok_1 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors separate_ok_2" 0.0 "bin/ext_transform_feedback-api-errors separate_ok_2 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors separate_unbound_0_1" 0.0 "bin/ext_transform_feedback-api-errors separate_unbound_0_1 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors separate_unbound_0_2" 0.0 "bin/ext_transform_feedback-api-errors separate_unbound_0_2 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors separate_unbound_1_2" 0.0 "bin/ext_transform_feedback-api-errors separate_unbound_1_2 -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors useprog_active" 0.0 "bin/ext_transform_feedback-api-errors useprog_active -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors useprogstage_active" 0.0 "bin/ext_transform_feedback-api-errors useprogstage_active -fbo -auto"
run_test "spec/EXT_transform_feedback/api-errors useprogstage_noactive" 0.0 "bin/ext_transform_feedback-api-errors useprogstage_noactive -fbo -auto"
run_test "spec/EXT_transform_feedback/buffer-usage" 0.0 "bin/ext_transform_feedback-buffer-usage -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_ClipDistance" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_ClipDistance -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_ClipDistance[1]-no-subscript" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_ClipDistance[1]-no-subscript -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_ClipDistance[2]-no-subscript" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_ClipDistance[2]-no-subscript -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_ClipDistance[3]-no-subscript" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_ClipDistance[3]-no-subscript -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_ClipDistance[4]-no-subscript" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_ClipDistance[4]-no-subscript -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_ClipDistance[5]-no-subscript" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_ClipDistance[5]-no-subscript -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_ClipDistance[6]-no-subscript" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_ClipDistance[6]-no-subscript -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_ClipDistance[7]-no-subscript" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_ClipDistance[7]-no-subscript -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_ClipDistance[8]-no-subscript" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_ClipDistance[8]-no-subscript -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_ClipVertex" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_ClipVertex -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_Color" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_Color -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_FogFragCoord" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_FogFragCoord -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_PointSize" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_PointSize -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_Position" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_Position -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_SecondaryColor" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_SecondaryColor -fbo -auto"
run_test "spec/EXT_transform_feedback/builtin-varyings gl_TexCoord" 0.0 "bin/ext_transform_feedback-builtin-varyings gl_TexCoord -fbo -auto"
run_test "spec/EXT_transform_feedback/change-size base-grow" 0.0 "bin/ext_transform_feedback-change-size base-grow -fbo -auto"
run_test "spec/EXT_transform_feedback/change-size base-shrink" 0.0 "bin/ext_transform_feedback-change-size base-shrink -fbo -auto"
run_test "spec/EXT_transform_feedback/change-size offset-grow" 0.0 "bin/ext_transform_feedback-change-size offset-grow -fbo -auto"
run_test "spec/EXT_transform_feedback/change-size offset-shrink" 0.0 "bin/ext_transform_feedback-change-size offset-shrink -fbo -auto"
run_test "spec/EXT_transform_feedback/change-size range-grow" 0.0 "bin/ext_transform_feedback-change-size range-grow -fbo -auto"
run_test "spec/EXT_transform_feedback/change-size range-shrink" 0.0 "bin/ext_transform_feedback-change-size range-shrink -fbo -auto"
run_test "spec/EXT_transform_feedback/discard-api" 0.0 "bin/ext_transform_feedback-discard-api -fbo -auto"
run_test "spec/EXT_transform_feedback/discard-bitmap" 0.0 "bin/ext_transform_feedback-discard-bitmap -fbo -auto"
run_test "spec/EXT_transform_feedback/discard-clear" 0.0 "bin/ext_transform_feedback-discard-clear -fbo -auto"
run_test "spec/EXT_transform_feedback/discard-copypixels" 0.0 "bin/ext_transform_feedback-discard-copypixels -fbo -auto"
run_test "spec/EXT_transform_feedback/discard-drawarrays" 0.0 "bin/ext_transform_feedback-discard-drawarrays -fbo -auto"
run_test "spec/EXT_transform_feedback/discard-drawpixels" 0.0 "bin/ext_transform_feedback-discard-drawpixels -fbo -auto"
run_test "spec/EXT_transform_feedback/generatemipmap buffer" 0.0 "bin/ext_transform_feedback-generatemipmap buffer -fbo -auto"
run_test "spec/EXT_transform_feedback/generatemipmap discard" 0.0 "bin/ext_transform_feedback-generatemipmap discard -fbo -auto"
run_test "spec/EXT_transform_feedback/generatemipmap prims_written" 0.0 "bin/ext_transform_feedback-generatemipmap prims_written -fbo -auto"
run_test "spec/EXT_transform_feedback/get-buffer-state buffer_size" 0.0 "bin/ext_transform_feedback-get-buffer-state buffer_size -fbo -auto"
run_test "spec/EXT_transform_feedback/get-buffer-state buffer_start" 0.0 "bin/ext_transform_feedback-get-buffer-state buffer_start -fbo -auto"
run_test "spec/EXT_transform_feedback/get-buffer-state indexed_binding" 0.0 "bin/ext_transform_feedback-get-buffer-state indexed_binding -fbo -auto"
run_test "spec/EXT_transform_feedback/get-buffer-state main_binding" 0.0 "bin/ext_transform_feedback-get-buffer-state main_binding -fbo -auto"
run_test "spec/EXT_transform_feedback/immediate-reuse" 0.0 "bin/ext_transform_feedback-immediate-reuse -fbo -auto"
run_test "spec/EXT_transform_feedback/interleaved-attribs" 0.0 "bin/ext_transform_feedback-interleaved -fbo -auto"
run_test "spec/EXT_transform_feedback/max-varyings" 0.0 "bin/ext_transform_feedback-max-varyings -fbo -auto"
popd

if [ $need_pass == 0 ] ; then
  echo "+---------------------------------------------+"
  echo "| Overall pass, as all 86 tests have passed. |"
  echo "+---------------------------------------------+"
else
  echo "+-----------------------------------------------------------+"
  echo "| Overall failure, as $need_pass tests did not pass and $failures failed. |"
  echo "+-----------------------------------------------------------+"
fi
exit $need_pass

