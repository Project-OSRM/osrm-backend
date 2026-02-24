"""This is a simplified version of e2e_benchmark.py.

e2e_benchmark.py uses bootstrapping, which is a procedure that obtains new data sets by
resampling an old data set, when obtaining new data sets may be hard or expensive. But
since we can obtain as many data sets as we like, bootstrapping makes no sense.

We run N random queries against the server. Since the queries are randomly generated the
response times obtained are meaningless except to compare them against the times
obtained by other PRs.

"""

import argparse
from collections import defaultdict
import csv
import gc
import gzip
import os
import platform
import random
import time
from statistics import NormalDist

import numpy as np
import requests


class BenchmarkRunner:
    def __init__(self, gps_traces):
        self.coordinates = []
        self.tracks = defaultdict(list)

        gps_traces_file_path = os.path.expanduser(gps_traces)

        if gps_traces_file_path.endswith(".gz"):
            infile = gzip.open(gps_traces_file_path, "rt")
        else:
            infile = open(gps_traces_file_path, "rt")

        with infile as file:
            reader = csv.DictReader(file)
            for row in reader:
                coord = (float(row["Latitude"]), float(row["Longitude"]))
                self.coordinates.append(coord)
                self.tracks[row["TrackID"]].append(coord)
        self.track_ids = list(self.tracks.keys())

    def make_url(self, host, benchmark_name):
        def toString(coords) -> str:
            return ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in coords])

        url = f"{host}/{benchmark_name}/v1/driving"

        if benchmark_name == "route":
            coords = random.sample(self.coordinates, 2)
            return f"{url}/{toString(coords)}?overview=full&steps=true"
        elif benchmark_name == "nearest":
            coords = random.sample(self.coordinates, 1)
            return f"{url}/{toString(coords)}"
        elif benchmark_name == "table":
            num_coords = random.randint(3, 12)
            coords = random.sample(self.coordinates, num_coords)
            return f"{url}/{toString(coords)}"
        elif benchmark_name == "trip":
            num_coords = random.randint(2, 10)
            coords = random.sample(self.coordinates, num_coords)
            return f"{url}/{toString(coords)}?steps=true"
        elif benchmark_name == "match":
            num_coords = random.randint(50, 100)
            track_id = random.choice(self.track_ids)
            coords = self.tracks[track_id][:num_coords]
            radiuses_str = ";".join(
                [f"{random.randint(10, 20)}" for _ in range(len(coords))]
            )
            return f"{url}/{toString(coords)}?steps=true&radiuses={radiuses_str}"
        else:
            raise Exception(f"Unknown benchmark: {benchmark_name}")

    def run(self, samples: np.ndarray, args) -> None:

        # See: https://peps.python.org/pep-0564/#windows
        t = (
            time.perf_counter_ns
            if platform.system() == "Windows"
            else time.process_time_ns
        )

        # each iteration has to get the same queries, or we will compare apples with
        # oranges!
        random.seed(42)
        reqs = [
            [self.make_url(args.host, args.method), ""] for j in range(args.requests)
        ]

        for i in range(-args.warmup_iterations, args.iterations):
            # gc.collect()
            gc.disable()
            start_time = t()
            for req in reqs:
                req[1] = requests.get(req[0])
            end_time = t()
            gc.enable()

            for req in reqs:
                url, response = req
                if response.status_code != 200:
                    code = response.json()["code"]
                    if code not in ["NoSegment", "NoMatch", "NoRoute", "NoTrips"]:
                        raise Exception(
                            f"Error: {url} {response.status_code} {response.text}"
                        )
            if i >= 0:
                samples[i] = end_time - start_time


def confidence_interval(data, confidence=0.95):
    dist = NormalDist.from_samples(data)
    z = NormalDist().inv_cdf((1 + confidence) / 2.0)
    h = dist.stdev * z / ((len(data) - 1) ** 0.5)
    return dist.mean, h


def main():
    parser = argparse.ArgumentParser(
        description="""
    Run E2E benchmark tests.

    This test reports the total execution time of a set of queries. The queries are
    randomly generated once, and the query set is then repeatedly sent to osrm-routed.
    The total times for each set are averaged and a confidence interval is calculated.

    osrm-routed must already be running with a dataset covering the given GPS traces.
    """
    )

    parser.add_argument(
        "--host",
        type=str,
        # "localhost" has abysmal performance on Windows
        default="http://127.0.0.1:5000",
        help="Host URL (http://127.0.0.1:5000)",
    )
    parser.add_argument(
        "--method",
        type=str,
        choices=["route", "table", "match", "nearest", "trip"],
        default="route",
        help="Benchmark method",
    )
    parser.add_argument(
        "--iterations",
        type=int,
        help="Number of iterations to make (20)",
        default=20,
    )
    parser.add_argument(
        "--requests",
        type=int,
        help="Number of requests per iteration (100)",
        default=100,
    )
    parser.add_argument(
        "--warmup-iterations",
        type=int,
        help="Number of warmup iterations to make (5)",
        default=5,
    )
    parser.add_argument(
        "--gps_traces",
        type=str,
        required=True,
        help="Path to the GPS traces file (.gz allowed)",
    )

    args = parser.parse_args()

    times = np.ndarray(args.iterations)
    runner = BenchmarkRunner(args.gps_traces)
    runner.run(times, args)

    ms = times / 1e6
    mean, h = confidence_interval(ms)

    print(f"{mean:.2f} ± {h:.2f}")


if __name__ == "__main__":
    main()
