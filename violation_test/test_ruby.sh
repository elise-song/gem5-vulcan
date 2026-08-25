#!/bin/bash
# Run gem5 with test asm; One run at a time!
# Currently configured for: InvisiSpec

export CODE_DIR=/code;
export RVZR_DIR=$CODE_DIR/revizor-docker;
export GEM5_DIR=$CODE_DIR/gem5-docker;

# Revizor priming flags: "Squashed,DRAM,CommMonitor,O3CPUAll,ExecAll,Branch,MemoryAccess,RubyCache,LSQ,LSQUnit,Fetch,Decode,RubySlicc,RubyPort,RubySequencer,RubyQueue,RubyNetwork"
# Must at least have "Squashed" !!!
export GEM5_DEBUG_FLAGS="Squashed,DRAM,CommMonitor,O3CPUAll,ExecAll,Branch,MemoryAccess,RubyCache,LSQ,LSQUnit,Fetch,Decode,RubySlicc,RubyPort,RubySequencer,RubyQueue,RubyNetwork";

# UnsafeBaseline, SpectreSafeInvisibleSpec
export INVISISPEC_SCHEME="SpectreSafeInvisibleSpec";

init_dirs(){ 
  RUN_NAME=$1;
  cd $GEM5_DIR/violation_test;
  mkdir -p $GEM5_DIR/violation_test/out;
  mkdir -p $GEM5_DIR/m5out_$RUN_NAME;
  mkdir -p $GEM5_DIR/checkpoint_$RUN_NAME;
}

compile_test_case() {
  RUN_ASM=$1; # Filepath
  echo "Compiling $RUN_ASM";
  cd $GEM5_DIR/violation_test;

  as -g -mmnemonic=intel -msyntax=intel $RUN_ASM -o ./out/test_case_$RUN_NAME.o;
  objcopy --remove-section .note.gnu.property ./out/test_case_$RUN_NAME.o;
  ld ./out/test_case_$RUN_NAME.o -o ./out/test_case_$RUN_NAME.out  -T ./link1.ld;
  objdump -d ./out/test_case_$RUN_NAME.out > ./out/test_case_$RUN_NAME.dump;

  echo "Done compiling $RUN_ASM";
}

memory_trace_run() {
  RUN_NAME=$1;
  CURR_SCHEME=$2;
  CURR_DEBUG_FLAGS=$3;

  echo "Running test_case_$RUN_NAME"
  cd $GEM5_DIR;
  mkdir -p m5out_$RUN_NAME
  $GEM5_DIR/build/X86/gem5.opt \
    --outdir=m5out_$RUN_NAME \
    --debug-flags=$CURR_DEBUG_FLAGS \
    --debug-file=log.out \
    --debug-start=0 \
    $GEM5_DIR/configs/example/se.py \
    --cmd=$GEM5_DIR/violation_test/out/test_case_$RUN_NAME.out \
    --cpu-type=DerivO3CPU --l1d_size=32kB --l1i_size=32kB \
    --l2cache --l1traces --ruby --caches --num-cpu=1 \
    --needsTSO=0 --scheme=$CURR_SCHEME \
    --rel-max-tick=5000000 \
    --checkpoint-at-end \
    --checkpoint-dir=$GEM5_DIR/checkpoint_$RUN_NAME > m5out_$RUN_NAME/gem5_output.out # Use &> to include stderr
  cd $GEM5_DIR/violation_test;
  echo "Done running test_case_$RUN_NAME"
}

final_cache_run() {
  RUN_NAME=$1;
  CURR_SCHEME=$2;
  CURR_DEBUG_FLAGS=$3;

  echo "Running test_case_$RUN_NAME"
  cd $GEM5_DIR;
  mkdir -p m5out_$RUN_NAME
  $GEM5_DIR/build/X86/gem5.opt \
    --outdir=m5out_$RUN_NAME \
    --debug-flags=$CURR_DEBUG_FLAGS \
    --debug-file=log.out \
    --debug-start=0 \
    $GEM5_DIR/configs/example/se.py \
    --cmd=$GEM5_DIR/violation_test/out/test_case_$RUN_NAME.out \
    --cpu-type=DerivO3CPU --l1d_size=32kB --l1i_size=32kB \
    --l2cache --ruby --dump-caches --caches --num-cpu=1 \
    --needsTSO=0 --scheme=$CURR_SCHEME \
    --checkpoint-at-end \
    --checkpoint-dir=$GEM5_DIR/checkpoint_$RUN_NAME > m5out_$RUN_NAME/gem5_output.out # Use &> to include stderr
  cd $GEM5_DIR/violation_test;
  echo "Done running test_case_$RUN_NAME"
}

init_run() {
  cd $GEM5_DIR/violation_test;
  RUN_ASM_FILENAME=$(basename $RUN_ASM);
  FILENAME_ARR=(${RUN_ASM_FILENAME//_/ });
  LEN=${#FILENAME_ARR[@]};

  for (( i=2; i<${LEN}; i++ )) # Format out "test_case"
  do
    RUN_NAME=${RUN_NAME}_${FILENAME_ARR[$i]};
  done
  export RUN_NAME=${RUN_NAME:1:-4};
  echo "Running $RUN_NAME";

  init_dirs $RUN_NAME;
  compile_test_case $RUN_ASM;
}

post_run() {
  RUN_ASM=$1
  RUN_NAME=$2;
  TRACE_MODE=$3;
  DEBUG_FLAGS=$4;

  echo "Post-processing $RUN_NAME";
  cd $GEM5_DIR/violation_test;
  M5OUT_DIR=$GEM5_DIR/m5out_$RUN_NAME;

  # Get packet traces if they exist
  if [[ "$TRACE_MODE" == "memory_trace" ]]; then
    python3.11 ./parse_traces.py $M5OUT_DIR;
  fi;
  
  # Copy run asm to M5OUT_DIR (Copy original, move temp files!)
  cp $RUN_ASM $M5OUT_DIR;

  # Move .o/.out/.dump from violation_test/out to M5OUT_DIR
  mv out/test_case_$RUN_NAME.* $M5OUT_DIR;

  # Move checkpoints to M5OUT_DIR (Important for final_cache!)
  rm -rf $M5OUT_DIR/checkpoint_$RUN_NAME;
  mv $GEM5_DIR/checkpoint_$RUN_NAME $M5OUT_DIR;

  # Touch the violation_test.yaml into M5OUT_DIR
  python3.11 ./config_gen.py $TRACE_MODE $DEBUG_FLAGS $M5OUT_DIR;
}

main() {
  ARG=$1; # trace_mode
  RUN_ASM=$2;
  SCHEME=${3:-$INVISISPEC_SCHEME}
  DEBUG_FLAGS=${4:-$GEM5_DEBUG_FLAGS} # Use $GEM5_DEBUG_FLAGS iff var not set
  RUN_NAME='';

  echo "Using debug flags: $DEBUG_FLAGS";

  cd $GEM5_DIR/violation_test;
  case $ARG in
    "compile_gem5")
      cd $GEM5_DIR;
      export CORES=$(( `nproc --all` + 1));
      python2.7 `which scons` -j${CORES} --verbose build/X86/gem5.opt --default=X86 PROTOCOL=MESI_Two_Level --ignore-style
    ;;

    "memory_trace")
      echo "Running with memory_trace";

      init_run $RUN_ASM;
      memory_trace_run $RUN_NAME $SCHEME $DEBUG_FLAGS;
      post_run $RUN_ASM $RUN_NAME $ARG $DEBUG_FLAGS;

      echo "Done running with memory_trace";
    ;;

    "final_cache")
      echo "Running with final_cache";

      init_run $RUN_ASM;
      final_cache_run $RUN_NAME $SCHEME $DEBUG_FLAGS;
      post_run $RUN_ASM $RUN_NAME $ARG $DEBUG_FLAGS;
      # final_cache cache_tags in $M5OUT_DIR/checkpoint_$RUN_NAME

      echo "Done running with final_cache";
    ;;

    *)
      echo "Error: No arguments given!";
      echo """
        Usage:
        ./test_ruby.sh compile_gem5
        ./test_ruby.sh <trace_mode> <run_asm> <invisispec_scheme> <debug_flags>

        Default Arguments (Optional):
        <invisispec_scheme=$INVISISPEC_SCHEME> 
        <debug_flags=$GEM5_DEBUG_FLAGS>

        Output: 
        m5out_<run_name> dir in gem5 root
          - Also contains .o/.out/.dump and violation_test.yaml
        
        <trace_mode> is "memory_trace" or "final_cache"
          - violation_test.yaml containing <trace_mode> and <debug_flags> touched into m5out
          TODO: "all" (both memory_trace and final_cache)

        <run_asm> is a filepath, expects format of './test_case_<run_name>.asm'

        <invisispec_scheme> can be "UnsafeBaseline", "SpectreSafeInvisibleSpec"

        <debug_flags> are gem5 debug flags

      """
    ;;
  esac
}

main "$@"