from locust import HttpUser, TaskSet, task, between
import csv
import random

class OSRMTasks(TaskSet):
    def on_start(self):
        random.seed(42)

        self.coordinates = []
        with open('~/gps_traces.csv', 'r') as file:
            reader = csv.DictReader(file)
            for row in reader:
                self.coordinates.append((row['Latitude'], row['Longitude']))

    @task
    def get_route(self):
        start = random.choice(self.coordinates)
        end = random.choice(self.coordinates)
        
        start_coord = f"{start[1]},{start[0]}"
        end_coord = f"{end[1]},{end[0]}"
        
        self.client.get(f"/route/v1/driving/{start_coord};{end_coord}?overview=full&steps=true")

    @task
    def get_table(self):
        num_coords = random.randint(3, 500)
        selected_coords = random.sample(self.coordinates, num_coords)
        coords_str = ";".join([f"{coord[1]},{coord[0]}" for coord in selected_coords])
        
        self.client.get(f"/table/v1/driving/{coords_str}")

class OSRMUser(HttpUser):
    tasks = [OSRMTasks]
    wait_time = between(0.01, 0.1)

