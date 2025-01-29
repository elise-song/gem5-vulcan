# Args: <trace_mode> <debug_flags> <output_dir>
#   Inputs: <trace_mode> <debug_flags>
#   Outputs: violation_test.yaml; Placed into output_dir

import yaml, sys, os

def main():
  assert sys.argv[1] != "", "trace_mode required"
  assert sys.argv[2] != "", "debug_flags required"
  assert sys.argv[3] != "", "output_dir required"

  output_dir = sys.argv[3]
  if not os.path.isdir(output_dir):
    print(f"Curr working dir is: {os.getcwd()}")
    exit(f"config_gen: Expected output directory {output_dir} not found!")
  print(f"Creating yaml in directory: {output_dir}")

  data : dict = [
     {"trace_mode" : sys.argv[1]},
     {"debug_flags" : sys.argv[2]}
    ]

  yaml_path = os.path.join(output_dir, "violation_test.yaml")
  with open(yaml_path, "w") as yaml_file:
    yaml.dump(data, yaml_file, sort_keys=False)

  print(f"Created {yaml_path}")
  return True

main()