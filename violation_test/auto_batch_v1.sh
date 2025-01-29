# Automatically run one batch after another!

export CODE_DIR=/code;
export RVZR_DIR=$CODE_DIR/revizor-docker;
export GEM5_DIR=$CODE_DIR/gem5-docker;
export VIOLATION_TEST_DIR=$GEM5_DIR/violation_test;
export TESTS_DEST=$VIOLATION_TEST_DIR/tests;
export TEST_RUBY_SH=$VIOLATION_TEST_DIR/test_ruby.sh;

# Usage
# ./auto_batch_v1.sh --src raw_dumps/IS-ST-256MSHR-CT_SEQ
# ./auto_batch_v1.sh --src raw_dumps/IS-ST-256MSHR-CT_SEQ --dst IS-ST
# ./auto_batch_v1.sh --src raw_dumps/IS-ST-256MSHR-CT_SEQ --verbose


# Define defaults
SRC_FOLDER_NAME="IS-ST-256MSHR-CT_SEQ-NO_L1I"
DEST_FOLDER_NAME="IS-ST"
VERBOSE=0;

# Parse input argument
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        -src|--src)
            SRC_FOLDER_NAME="$2"
            shift
            shift
            ;;
	-dst|--dst)
            DEST_FOLDER_NAME="$2"
            shift
            shift
            ;;
	-verbose|--verbose)
            VERBOSE=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo "Running multiple batches";
cd $VIOLATION_TEST_DIR;

######
# FUNCTIONS
######

# Copy violations from $SRC_FOLDER_NAME into $VIOLATION_TEST_DIR/fuzz_results_raw
copy_dirs() {    
    # Step 1: Remove contents of violation_test/fuzz_results_raw
    rm -rf $VIOLATION_TEST_DIR/fuzz_results_raw/*

    # Step 2: Copy all dirs from violation_test/raw_dumps/IS-ST-256MSHR-CT_SEQ-NO_L1I into fuzz_results_raw
    cp -R $VIOLATION_TEST_DIR/$SRC_FOLDER_NAME/* $VIOLATION_TEST_DIR/fuzz_results_raw/
}

# Execute tests
auto_batch() {
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

# Example usage: check_violation "IS-ST"
check_violation() {
    outdir="$1"

    # Check if test_results_"$outdir" exists
    if [ ! -d "test_results_$outdir" ]; then
        echo "Directory test_results_$outdir does not exist."
        return
    fi

    # Loop through each directory in test_results_"$outdir"
    for DIRNAME in test_results_"$outdir"/*; do
	input_violation=0
	output_violation=0
	
        if [ -d "$DIRNAME" ]; then
	    # Check if original violation existed.
	    cache_diff=0
            echo "Processing $DIRNAME"
            input_diff=$(diff "fuzz_results_raw/${DIRNAME##*/}/CTrace_input1.out" "fuzz_results_raw/${DIRNAME##*/}/CTrace_input2.out" 2>/dev/null)
            if [ $? -ne 0 ]; then
                echo -e "\e[31mERROR\e[0m: CT different in Input: $DIRNAME"
		if [ $VERBOSE -eq 1 ] ; then 
                    echo -e "\e[31m$input_diff\e[0m"
		fi
            else

		if [ $VERBOSE -eq 1 ] ; then 		    
		    echo -e "\e[32mCT same in Input: $DIRNAME\e[0m"
		fi
		
                cache_diff=$(diff <(tail -n +2 "fuzz_results_raw/${DIRNAME##*/}/cache_tags_1") <(tail -n +2 "fuzz_results_raw/${DIRNAME##*/}/cache_tags_2") 2>/dev/null)
                if [ $? -eq 0 ]; then
                    echo -e "\e[31mERROR\e[0m: Cache tags match in $DIRNAME"
                else
		if [ $VERBOSE -eq 1 ] ; then 		    
                    echo -e "\e[32mCache tags differ in Input: $DIRNAME\e[0m"
                    echo -e "\e[32m$cache_diff\e[0m"
		fi
		    input_violation=1
                fi
            fi

	    # Check if re-run output has violation.
            output_diff=$(diff <(tail -n +2 ${DIRNAME}/m5out_*a/checkpoint_*/tags.*/m5.tag) <(tail -n +2 ${DIRNAME}/m5out_*b/checkpoint_*/tags.*/m5.tag) 2>/dev/null)

            if [ $? -eq 0 ]; then
                echo -e "\e[31mERROR\e[0m: No difference in Cache Tags in Output: $DIRNAME"
            else
		if [ $VERBOSE -eq 1 ] ; then 		    		    
                    echo -e "\e[32mOutput violation in $DIRNAME\e[0m"
                    echo -e "\e[32m$output_diff\e[0m"
		fi
		output_violation=1
            fi

	    if [ "$input_violation" -ne 0 ] && [ "$output_violation" -ne 0 ]; then
		echo -e "\e[32mReal violation\e[0m: $DIRNAME"
            else
                echo -e "\e[31mFalse violation\e[0m: $DIRNAME"
            fi
        fi

	echo "---------------------"
	
    done
}


######
# RUN
######

# setup
copy_dirs

# run auto batch
export -f auto_batch;
bash -c auto_batch &> auto_batch_output.out &

# Get the PID of the last background command and wait for it to finish.
pid=$!
wait $pid

# Check the exit status of the process
if [ $? -eq 0 ]; then
    echo "Background process finished successfully"
else
    echo "Background process finished with an error"
fi

# analyze
check_violation $DEST_FOLDER_NAME

echo "Done running multiple batches";
