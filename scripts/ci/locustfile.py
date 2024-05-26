from locust import HttpUser, TaskSet, task, between, events
import csv
import random
from collections import defaultdict
import os

class OSRMTasks(TaskSet):
    def on_start(self):
        random.seed(42)

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

    @task
    def get_route(self):
        start = random.choice(self.coordinates)
        end = random.choice(self.coordinates)
        
        start_coord = f"{start[1]:.6f},{start[0]:.6f}"
        end_coord = f"{end[1]:.6f},{end[0]:.6f}"
        
        self.client.get(f"/route/v1/driving/{start_coord};{end_coord}?overview=full&steps=true", name="/route/v1/driving")

    # @task
    # def get_table(self):
    #     num_coords = random.randint(3, 250)
    #     selected_coords = random.sample(self.coordinates, num_coords)
    #     coords_str = ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in selected_coords])
        
    #     self.client.get(f"/table/v1/driving/{coords_str}")

    # @task
    # def get_match(self):
    #     num_coords = random.randint(3, 250)
    #     track_id = random.choice(self.track_ids)
    #     track_coords = self.tracks[track_id][:num_coords]
    #     coords_str = ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in track_coords])
        
    #     self.client.get(f"/match/v1/driving/{coords_str}?steps=true")

    # @task
    # def get_nearest(self):
    #     coord = random.choice(self.coordinates)
    #     coord_str = f"{coord[1]:.6f},{coord[0]:.6f}"
        
    #     self.client.get(f"/nearest/v1/driving/{coord_str}")

    # @task
    # def get_trip(self):
    #     num_coords = random.randint(2, 10)
    #     selected_coords = random.sample(self.coordinates, num_coords)
    #     coords_str = ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in selected_coords])
        
    #     self.client.get(f"/trip/v1/driving/{coords_str}?steps=true")

class OSRMUser(HttpUser):
    tasks = [OSRMTasks]
    wait_time = between(0.01, 0.1)

    # @events.quitting.add_listener
    # def _(environment, **kwargs):
    #     def calculate_percentiles(stats, percentiles):
    #         response_times = sorted(stats.get_response_times())
    #         percentile_values = {}
    #         for percentile in percentiles:
    #             rank = int(len(response_times) * (percentile / 100))
    #             percentile_values[percentile] = response_times[rank-1] if rank > 0 else 0
    #         return percentile_values
        
    #     nearest_stats = environment.stats.get("/nearest/v1/driving", "GET")
    #     route_stats = environment.stats.get("/route/v1/driving", "GET")
        
    #     nearest_percentiles = calculate_percentiles(nearest_stats, [95, 99])
    #     route_percentiles = calculate_percentiles(route_stats, [95, 99])
        
    #     print("\nAggregated Statistics for /nearest/v1/driving:")
    #     print(f"Request Count: {nearest_stats.num_requests}")
    #     print(f"Failure Count: {nearest_stats.num_failures}")
    #     print(f"Median Response Time: {nearest_stats.median_response_time}")
    #     print(f"Average Response Time: {nearest_stats.avg_response_time}")
    #     print(f"Min Response Time: {nearest_stats.min_response_time}")
    #     print(f"Max Response Time: {nearest_stats.max_response_time}")
    #     print(f"Average Content Size: {nearest_stats.avg_content_length}")
    #     print(f"p95 Response Time: {nearest_percentiles[95]}")
    #     print(f"p99 Response Time: {nearest_percentiles[99]}")
        
    #     print("\nAggregated Statistics for /route/v1/driving:")
    #     print(f"Request Count: {route_stats.num_requests}")
    #     print(f"Failure Count: {route_stats.num_failures}")
    #     print(f"Median Response Time: {route_stats.median_response_time}")
    #     print(f"Average Response Time: {route_stats.avg_response_time}")
    #     print(f"Min Response Time: {route_stats.min_response_time}")
    #     print(f"Max Response Time: {route_stats.max_response_time}")
    #     print(f"Average Content Size: {route_stats.avg_content_length}")
    #     print(f"p95 Response Time: {route_percentiles[95]}")
    #     print(f"p99 Response Time: {route_percentiles[99]}")

