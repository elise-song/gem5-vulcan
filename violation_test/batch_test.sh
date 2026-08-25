#!/bin/bash
# Batch run ./test_ruby.sh
# Run all tests in violation_test/batch/tests

export CODE_DIR=/code;
export RVZR_DIR=$CODE_DIR/revizor-docker;
export GEM5_DIR=$CODE_DIR/gem5-docker;
export VIOLATION_TEST_DIR=$GEM5_DIR/violation_test;
export TESTS_DEST=$VIOLATION_TEST_DIR/tests;
export TEST_RUBY_SH=$VIOLATION_TEST_DIR/test_ruby.sh;


### Current Run Config ###
# Only affects runs!

# Revizor priming flags: "Squashed,DRAM,CommMonitor,O3CPUAll,ExecAll,Branch,ROB,MemoryAccess,MemDepUnit,RubyCache,LSQ,LSQUnit,Fetch,Decode,RubySlicc,RubyPort,RubySequencer,RubyQueue,RubyNetwork,CacheAccess,IEW"
# General Debug Flags: "Squashed,ExecAll,ROB,LSQ,LSQUnit,MemDepUnit,Branch,Fetch,IQ,LSQ,LSQUnit"
# L1I debug flags: "Squashed,Fetch,IQ,RubyCache"
# MSHR debug flags: "Squashed,Fetch,IQ,LSQ,LSQUnit,MemDepUnit"
# Must at least have "Squashed" !!!
export GEM5_DEBUG_FLAGS="Squashed,ExecAll,ROB,LSQ,LSQUnit,MemDepUnit,Branch,Fetch,IQ,LSQ,LSQUnit";

# UnsafeBaseline, SpectreSafeInvisibleSpec
export DEFAULT_SCHEME="SpectreSafeInvisibleSpec";

# memory_trace, final_cache
export DEFAULT_TRACE_MODE="memory_trace";

#########


main() {
  ARG=$1 # Set initial state

  cd $VIOLATION_TEST_DIR;
  case $ARG in
    "compile_gem5")
      cd $GEM5_DIR;
      export CORES=$(( `nproc --all` + 1));
      python2.7 `which scons` -j${CORES} --verbose build/X86/gem5.opt --default=X86 PROTOCOL=MESI_Two_Level --ignore-style
    ;;

    "process_results")
      # Expects raw m5out dirs in format 00..0hrs-{0}0mins-{0}0secs in $VIOLATION_TEST_DIR/fuzz_results_raw
      PROCESSED_DIR=$VIOLATION_TEST_DIR/fuzz_processed;
      PROCESS_ALL_RESULTS=$2;
      echo "Processing fuzzing results";

      cd $VIOLATION_TEST_DIR/fuzz_results_raw;
      for M5OUT_DIR in */ ; do
        DIR_NAME=${M5OUT_DIR:0:-1};
        echo "Processing fuzzing results for $DIR_NAME";

        if [[ -e $DIR_NAME/memory_packets_1.out || -e $DIR_NAME/memory_packets_2.out || $PROCESS_ALL_RESULTS == "all" ]]; then
          DEST_DIR=$PROCESSED_DIR/memory_trace/$DIR_NAME;
          mkdir -p $DEST_DIR;
          cp $DIR_NAME/test_case_input1.asm $DEST_DIR/test_case_${DIR_NAME}_a.asm;
          cp $DIR_NAME/test_case_input2.asm $DEST_DIR/test_case_${DIR_NAME}_b.asm;
        fi; # Copy asms into $PROCESSED_DIR/memory_trace/$DIR_NAME

        if [[ -e $DIR_NAME/cache_tags_1 || -e $DIR_NAME/cache_tags_2 || $PROCESS_ALL_RESULTS == "all" ]]; then
          DEST_DIR=$PROCESSED_DIR/final_cache/$DIR_NAME;
          mkdir -p $DEST_DIR;
          cp $DIR_NAME/test_case_input1.asm $DEST_DIR/test_case_${DIR_NAME}_a.asm;
          cp $DIR_NAME/test_case_input2.asm $DEST_DIR/test_case_${DIR_NAME}_b.asm;
        fi; # Copy asms into $PROCESSED_DIR/final_cache/$DIR_NAME; Both cases can occur!
      done;

      echo "Done processing fuzzing results";
    ;;

    "copy_tests")
      TESTS_SRC=$2;

      rm -rf $TESTS_DEST;
      mkdir $TESTS_DEST;

      echo "Copying tests from $TESTS_SRC";
      cd $TESTS_SRC;
      for TEST_DIR in */ ; do
        cp -vr $TEST_DIR $TESTS_DEST;
      done;
      echo "Done copying tests into $TESTS_DEST";
    ;;

    "run")
      TRACE_MODE=${2:-$DEFAULT_TRACE_MODE};
      SCHEME=${3:-$DEFAULT_SCHEME};

      TEST_RESULTS_DIR=$VIOLATION_TEST_DIR/test_results;
      rm -rf $TEST_RESULTS_DIR;
      mkdir $TEST_RESULTS_DIR;
      
      # Run all tests in $TESTS_DEST in parallel
      echo "Running all tests in $TESTS_DEST";

      cd $TESTS_DEST;
      for TEST_DIR in */ ; do
        DIR_NAME=${TEST_DIR:0:-1};
        echo -e "\nRunning tests in $DIR_NAME";
        TEST_CASE_A=$TESTS_DEST/$DIR_NAME/test_case_${DIR_NAME}_a.asm; # RUN_NAME=${DIR_NAME}_a
        TEST_CASE_B=$TESTS_DEST/$DIR_NAME/test_case_${DIR_NAME}_b.asm;

        # ./test_ruby.sh <trace_mode> <run_asm> <invisispec_scheme> <debug_flags>
        echo "Running test_case_a: $TEST_RUBY_SH $TRACE_MODE $TEST_CASE_A $SCHEME $GEM5_DEBUG_FLAGS"
        $TEST_RUBY_SH $TRACE_MODE $TEST_CASE_A $SCHEME $GEM5_DEBUG_FLAGS;
        echo "Running test_case_b: $TEST_RUBY_SH $TRACE_MODE $TEST_CASE_B $SCHEME $GEM5_DEBUG_FLAGS"
        $TEST_RUBY_SH $TRACE_MODE $TEST_CASE_B $SCHEME $GEM5_DEBUG_FLAGS;

        # Move results from $GEM5_DIR/m5out_$RUN_NAME into $VIOLATION_TEST_DIR/test_results
        echo "Moving m5out's for $DIR_NAME";
        mkdir $TEST_RESULTS_DIR/$DIR_NAME;
        mv $GEM5_DIR/m5out_${DIR_NAME}_a $TEST_RESULTS_DIR/$DIR_NAME;
        mv $GEM5_DIR/m5out_${DIR_NAME}_b $TEST_RESULTS_DIR/$DIR_NAME;

        echo -e "Done running for $DIR_NAME\n";
      done;

      echo "Done running tests"
    ;;

    *)
      echo "Error: No arguments given!";
      echo """
        **Using violation_test/test_ruby.sh**

        Usage:
        ./batch_test.sh compile_gem5

        ./batch_test.sh process_results (all)
          - Extracts test asms from fuzz_results_raw into e.g. fuzz_processed/memory_trace/123hrs-45mins-54secs/test_case_123hrs-45mins-54secs_a.asm

        ./batch_test.sh copy_tests <src_dir>
          - E.g. "./batch_test.sh copy_tests ./fuzz_processed/final_cache"
          - <src_dir> should have dir format e.g. 123hrs-45mins-54secs/test_case_123hrs-45mins-54secs_a.asm
          - Warning: Will wipe out existing batch_test/tests!

        ./batch_test.sh run <trace_mode=$DEFAULT_TRACE_MODE> <scheme=$DEFAULT_SCHEME>
          - Runs all asms in batch_test/tests and puts results into batch_test/test_results

        - batch_test/tests should have dir format: batch_test/tests/123hrs-45mins-54secs/test_case_123hrs-45mins-54secs_a.asm
        - batch_test/test_results should have dir format: batch_test/test_results/123hrs-45mins-54secs/m5out_123hrs-45mins-54secs_a/...

        Output: 
        m5out_<run_name> dirs moved from gem5 root into batch_test/test_results
        (Wiped on every new run!)

      """
    ;;
  esac
}

main "$@"