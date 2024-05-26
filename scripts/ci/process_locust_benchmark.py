import sys
import csv

def check_csv_is_empty(file_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()
        return len(lines) <= 1 

def main(locust_csv_base_name, prefix, output_folder):
    if not check_csv_is_empty(f"{locust_csv_base_name}_exceptions.csv") or not check_csv_is_empty(f"{locust_csv_base_name}_failures"):
        raise Exception("There are exceptions or failures in the locust benchmark")

    with open(f"{locust_csv_base_name}_stats.csv", 'r') as file:
            reader = csv.DictReader(file)
            for row in reader:
                name = row['Name']
                if name == 'Aggregated': continue
                
                statistics = f'''
Request Count: {row['Request Count']}
Requests/s: {row['Requests/s']}req/s
Failures/s: {row['Failures/s']}fail/s
avg: {row['Average Response Time']}ms
50%: {row['50%']}
75%: {row['75%']}ms
95%: {row['95%']}ms
98%: {row['98%']}ms
99%: {row['99%']}ms   
max: {row['Max Response Time']}ms
'''
                with open(f"{prefix}_{name}_{output_folder}.bench", 'w') as f:
                    f.write(statistics)

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <locust csv base name> <prefix> <output folder>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3])