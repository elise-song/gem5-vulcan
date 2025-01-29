# Automatically run one batch after another!

export CODE_DIR=/code;
export RVZR_DIR=$CODE_DIR/revizor-docker;
export GEM5_DIR=$CODE_DIR/gem5-docker;
export VIOLATION_TEST_DIR=$GEM5_DIR/violation_test;
export TESTS_DEST=$VIOLATION_TEST_DIR/tests;
export TEST_RUBY_SH=$VIOLATION_TEST_DIR/test_ruby.sh;

echo "Running multiple batches";
cd $VIOLATION_TEST_DIR;

# Current fuzz_results_raw: Violations from InvisiSpec Unsafebaseline, both memory_trace and final_cache runs

auto_batch() {
  # Manual: Copy result findings from $RVZR_DIR/src/results into $VIOLATION_TEST_DIR/fuzz_results_raw
  cd $VIOLATION_TEST_DIR;
  ./batch_test.sh compile_gem5;
  ./batch_test.sh process_results all;

  # Run all - USBL
  echo "Running all - USBL";
  ./batch_test.sh copy_tests ./fuzz_processed/final_cache;
  ./batch_test.sh run final_cache UnsafeBaseline;
  rm -rf test_results_USBL-ST; # Unsafebaseline-StateTrace
  mv test_results test_results_USBL-ST;
  ./batch_test.sh copy_tests ./fuzz_processed/memory_trace;
  ./batch_test.sh run memory_trace UnsafeBaseline;
  rm -rf test_results_USBL-TT; # Unsafebaseline-TimingTrace
  mv test_results test_results_USBL-TT;

  # Run all - IS
  echo "Running all - IS";
  ./batch_test.sh copy_tests ./fuzz_processed/final_cache;
  ./batch_test.sh run final_cache SpectreSafeInvisibleSpec;
  rm -rf test_results_IS-ST; # InvisiSpec-StateTrace
  mv test_results test_results_IS-ST;
  ./batch_test.sh copy_tests ./fuzz_processed/memory_trace;
  ./batch_test.sh run memory_trace SpectreSafeInvisibleSpec;
  rm -rf test_results_IS-TT; # InvisiSpec-TimingTrace
  mv test_results test_results_IS-TT;
}
export -f auto_batch;
bash -c auto_batch &> auto_batch_output.out &

echo "Done running multiple batches";