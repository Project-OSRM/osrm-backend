import requests
import random
from collections import defaultdict
import os
import csv
import numpy as np
import time
import argparse


class BenchmarkRunner:
    def __init__(self, gps_traces_file_path):
        self.coordinates = []
        self.tracks = defaultdict(list)

        gps_traces_file_path = os.path.expanduser(gps_traces_file_path)
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
                code = response.json()['code']
                if code in ['NoSegment', 'NoMatch', 'NoRoute', 'NoTrips']:
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
            num_coords = random.randint(3, 12)
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

def bootstrap_confidence_interval(data, num_samples=1000, confidence_level=0.95):
    means = []
    for _ in range(num_samples):
        sample = np.random.choice(data, size=len(data), replace=True)
        means.append(np.mean(sample))
    lower_bound = np.percentile(means, (1 - confidence_level) / 2 * 100)
    upper_bound = np.percentile(means, (1 + confidence_level) / 2 * 100)
    mean = np.mean(means)
    return mean, lower_bound, upper_bound

def calculate_confidence_interval(data, min_is_best=True):
    mean, lower, upper = bootstrap_confidence_interval(data)
    min_value = np.min(data) if min_is_best else np.max(data)
    return mean, (upper - lower) / 2, min_value


def main():
    parser = argparse.ArgumentParser(description='Run GPS benchmark tests.')
    parser.add_argument('--host', type=str, required=True, help='Host URL')
    parser.add_argument('--method', type=str, required=True, choices=['route', 'table', 'match', 'nearest', 'trip'], help='Benchmark method')
    parser.add_argument('--num_requests', type=int, required=True, help='Number of requests to perform')
    parser.add_argument('--iterations', type=int, required=True, help='Number of iterations to run the benchmark')
    parser.add_argument('--gps_traces_file_path', type=str, required=True, help='Path to the GPS traces file')

    args = parser.parse_args()

    np.random.seed(42)

    runner = BenchmarkRunner(args.gps_traces_file_path)
    
    all_times = []
    for _ in range(args.iterations):
        random.seed(42)
        times = runner.run(args.method, args.host, args.num_requests)
        all_times.append(times)
    all_times = np.asarray(all_times)

    assert all_times.shape == (args.iterations, all_times.shape[1])


    total_time, total_ci, total_best = calculate_confidence_interval(np.sum(all_times, axis=1))
    ops_per_sec, ops_per_sec_ci, ops_per_sec_best = calculate_confidence_interval(float(all_times.shape[1]) / np.sum(all_times / 1000, axis=1), min_is_best=False)
    min_time, min_ci, _ = calculate_confidence_interval(np.min(all_times, axis=1))
    mean_time, mean_ci, _ = calculate_confidence_interval(np.mean(all_times, axis=1))
    median_time, median_ci, _ = calculate_confidence_interval(np.median(all_times, axis=1))
    perc_95_time, perc_95_ci, _ = calculate_confidence_interval(np.percentile(all_times, 95, axis=1))
    perc_99_time, perc_99_ci, _ = calculate_confidence_interval(np.percentile(all_times, 99, axis=1))
    max_time, max_ci, _ = calculate_confidence_interval(np.max(all_times, axis=1))

    print(f'Ops: {ops_per_sec:.2f} ± {ops_per_sec_ci:.2f} ops/s. Best: {ops_per_sec_best:.2f} ops/s')
    print(f'Total: {total_time:.2f}ms ± {total_ci:.2f}ms. Best: {total_best:.2f}ms')
    print(f"Min time: {min_time:.2f}ms ± {min_ci:.2f}ms")
    print(f"Mean time: {mean_time:.2f}ms ± {mean_ci:.2f}ms")
    print(f"Median time: {median_time:.2f}ms ± {median_ci:.2f}ms")
    print(f"95th percentile: {perc_95_time:.2f}ms ± {perc_95_ci:.2f}ms")
    print(f"99th percentile: {perc_99_time:.2f}ms ± {perc_99_ci:.2f}ms")
    print(f"Max time: {max_time:.2f}ms ± {max_ci:.2f}ms")

if __name__ == '__main__':
    main()
