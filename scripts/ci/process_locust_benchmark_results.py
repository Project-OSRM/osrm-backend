import sys
import csv

def main(locust_csv_base_name, prefix, output_folder):
    with open(f"{locust_csv_base_name}_stats.csv", 'r') as file:
            reader = csv.DictReader(file)
            for row in reader:
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
                    f.write(statistics)

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <locust csv base name> <prefix> <output folder>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3])