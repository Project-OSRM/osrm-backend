import requests
import sys
import random
from collections import defaultdict
import os
import csv
import numpy as np
import time
import argparse

class BenchmarkRunner:
    def __init__(self):
        self.coordinates = []
        self.tracks = defaultdict(list)

        gps_traces_file_path = os.path.expanduser('~/gps_traces.csv')
        with open(gps_traces_file_path, 'r') as file:
            reader = csv.DictReader(file)
            for row in reader:
                coord = (float(row['Latitude']), float(row['Longitude']))
                self.coordinates.append(coord)
                self.tracks[row['TrackID']].append(coord)
        self.track_ids = list(self.tracks.keys())
    
    def run(self, benchmark_name, host, num_requests, warmup_requests=50):
        for _ in range(warmup_requests):
            url = self.make_url(host, benchmark_name)
            _ = requests.get(url)
    
        times = []

        for _ in range(num_requests):
            url = self.make_url(host, benchmark_name)

            start_time = time.time()
            response = requests.get(url)
            end_time = time.time()
            if response.status_code != 200:
                if benchmark_name == 'match':
                    code = response.json()['code']
                    if code == 'NoSegment' or code == 'NoMatch':
                        continue
                raise Exception(f"Error: {response.status_code} {response.text}")
            times.append((end_time - start_time) * 1000) # convert to ms
        
        return times
    
    def make_url(self, host, benchmark_name): 
        if benchmark_name == 'route':
            start = random.choice(self.coordinates)
            end = random.choice(self.coordinates)
            
            start_coord = f"{start[1]:.6f},{start[0]:.6f}"
            end_coord = f"{end[1]:.6f},{end[0]:.6f}"
            return f"{host}/route/v1/driving/{start_coord};{end_coord}?overview=full&steps=true"
        elif benchmark_name == 'table':
            num_coords = random.randint(3, 100)
            selected_coords = random.sample(self.coordinates, num_coords)
            coords_str = ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in selected_coords])
            return f"{host}/table/v1/driving/{coords_str}"
        elif benchmark_name == 'match':
            num_coords = random.randint(50, 100)
            track_id = random.choice(self.track_ids)
            track_coords = self.tracks[track_id][:num_coords]
            coords_str = ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in track_coords])
            radiues_str = ";".join([f"{random.randint(5, 20)}" for _ in range(len(track_coords))])
            return f"{host}/match/v1/driving/{coords_str}?steps=true&radiuses={radiues_str}"
        elif benchmark_name == 'nearest':
            coord = random.choice(self.coordinates)
            coord_str = f"{coord[1]:.6f},{coord[0]:.6f}"
            return f"{host}/nearest/v1/driving/{coord_str}"
        elif benchmark_name == 'trip':
            num_coords = random.randint(2, 10)
            selected_coords = random.sample(self.coordinates, num_coords)
            coords_str = ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in selected_coords])
            return f"{host}/trip/v1/driving/{coords_str}?steps=true"
        else:
            raise Exception(f"Unknown benchmark: {benchmark_name}")

def calculate_confidence_interval(data):
    assert len(data) == 5
    mean = np.mean(data)
    std_err = np.std(data, ddof=1) / np.sqrt(len(data))
    h = std_err * 1.96 # 95% confidence interval
    return mean, h

def main():
    parser = argparse.ArgumentParser(description='Run GPS benchmark tests.')
    parser.add_argument('--host', type=str, required=True, help='Host URL')
    parser.add_argument('--method', type=str, required=True, choices=['route', 'table', 'match', 'nearest', 'trip'], help='Benchmark method')
    parser.add_argument('--num_requests', type=int, required=True, help='Number of requests to perform')
    parser.add_argument('--iterations', type=int, default=5, required=True, help='Number of iterations to run the benchmark')

    args = parser.parse_args()


    runner = BenchmarkRunner()
    
    all_times = []
    for _ in range(args.iterations):
        random.seed(42)
        times = runner.run(args.method, args.host, args.num_requests)
        all_times.extend(times)
    
    total_time, total_ci = calculate_confidence_interval(np.sum(all_times, axis=0))
    min_time, min_ci = calculate_confidence_interval(np.min(all_times, axis=0))
    mean_time, mean_ci = calculate_confidence_interval(np.mean(all_times, axis=0))
    median_time, median_ci = calculate_confidence_interval(np.median(all_times, axis=0))
    perc_95_time, perc_95_ci = calculate_confidence_interval(np.percentile(all_times, 95, axis=0))
    perc_99_time, perc_99_ci = calculate_confidence_interval(np.percentile(all_times, 99, axis=0))
    max_time, max_ci = calculate_confidence_interval(np.max(all_times, axis=0))

    print(f'Total: {total_time}ms ± {total_ci:.2f}ms')
    print(f"Min time: {min_time}ms ± {min_ci:.2f}ms")
    print(f"Mean time: {mean_time:.2f}ms ± {mean_ci:.2f}ms")
    print(f"Median time: {median_time}ms ± {median_ci:.2f}ms")
    print(f"95th percentile: {perc_95_time:.2f}ms ± {perc_95_ci:.2f}ms")
    print(f"99th percentile: {perc_99_time:.2f}ms ± {perc_99_ci:.2f}ms")
    print(f"Max time: {max_time}ms ± {max_ci:.2f}ms")

if __name__ == '__main__':
    main()
