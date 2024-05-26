from locust import HttpUser, TaskSet, task, between
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
        
        self.client.get(f"/route/v1/driving/{start_coord};{end_coord}?overview=full&steps=true")

    @task
    def get_table(self):
        num_coords = random.randint(3, 500)
        selected_coords = random.sample(self.coordinates, num_coords)
        coords_str = ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in selected_coords])
        
        self.client.get(f"/table/v1/driving/{coords_str}")

    @task
    def get_match(self):
        track_id = random.choice(self.track_ids)
        track_coords = self.tracks[track_id]
        coords_str = ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in track_coords])
        
        self.client.get(f"/match/v1/driving/{coords_str}?steps=true")

    @task
    def get_nearest(self):
        coord = random.choice(self.coordinates)
        coord_str = f"{coord[1]:.6f},{coord[0]:.6f}"
        
        self.client.get(f"/nearest/v1/driving/{coord_str}")

    @task
    def get_trip(self):
        num_coords = random.randint(2, 10)
        selected_coords = random.sample(self.coordinates, num_coords)
        coords_str = ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in selected_coords])
        
        self.client.get(f"/trip/v1/driving/{coords_str}?steps=true")

class OSRMUser(HttpUser):
    tasks = [OSRMTasks]
    wait_time = between(0.01, 0.1)

