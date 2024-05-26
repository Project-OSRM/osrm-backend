import sys
import csv

def csv_is_empty(file_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()
        return len(lines) <= 1 

def main(locust_csv_base_name, prefix, output_folder):
    print(f"Processing locust benchmark results for {locust_csv_base_name}")
    if not csv_is_empty(f"{locust_csv_base_name}_exceptions.csv") or not csv_is_empty(f"{locust_csv_base_name}_failures.csv"):
        raise Exception("There are exceptions or failures in the locust benchmark")
    print(f"Locust benchmark {locust_csv_base_name} has no exceptions or failures")
    with open(f"{locust_csv_base_name}_stats.csv", 'r') as file:
        print('Stats:', file.read())
    
    with open(f"{locust_csv_base_name}_stats.csv", 'r') as file:
            reader = csv.DictReader(file)
            for row in reader:
                print('Processing row:', row )

                name = row['Name']
                if name == 'Aggregated': continue
                
                statistics = f'''
requests: {row['Request Count']}
req/s: {float(row['Requests/s']):.3f}req/s
fail/s: {float(row['Failures/s']):.3f}fail/s
avg: {float(row['Average Response Time']):.3f}ms
50%: {row['50%']}ms
75%: {row['75%']}ms
95%: {row['95%']}ms
98%: {row['98%']}ms
99%: {row['99%']}ms   
min: {float(row['Min Response Time']):.3f}ms
max: {float(row['Max Response Time']):.3f}ms
'''
                with open(f"{output_folder}/{prefix}_{name}.bench", 'w') as f:
                    print(f"Writing statistics to {output_folder}/{prefix}_{name}.bench")
                    f.write(statistics)

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <locust csv base name> <prefix> <output folder>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3])