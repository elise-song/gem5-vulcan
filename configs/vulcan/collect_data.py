import os
import glob

# Directory containing the report files
data_dir = 'configs/vulcan/data'

# Initialize sums
total_success = 0
total_cant_recover = 0
total_all_miss = 0
total_all_hit = 0
total_multiple_map = 0
total_other = 0
total_total = 0

# Find all .report.txt files
report_files = glob.glob(os.path.join(data_dir, '*.report.txt'))

for file_path in report_files:
    with open(file_path, 'r') as f:
        lines = f.readlines()
        for line in lines:
            parts = line.split(':', 1)
            if len(parts) == 2:
                key = parts[0].strip()
                value = parts[1].strip()
                if value.isdigit():
                    num = int(value)
                    if key == 'success':
                        total_success += num
                    elif key == 'cant recover set':
                        total_cant_recover += num
                    elif key == 'all miss':
                        total_all_miss += num
                    elif key == 'all hit':
                        total_all_hit += num
                    elif key == 'multiple map':
                        total_multiple_map += num
                    elif key == 'other':
                        total_other += num
                    elif key == 'total':
                        total_total += num

# Print the sums
print(f"Total success: {total_success}")
print(f"Total cant recover set: {total_cant_recover}")
print(f"Total all miss: {total_all_miss}")
print(f"Total all hit: {total_all_hit}")
print(f"Total multiple map: {total_multiple_map}")
print(f"Total other: {total_other}")
print(f"Total total: {total_total}")