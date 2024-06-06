import requests
import sys
import random
from collections import defaultdict
import os
import csv
import numpy as np

def main():
    pass

    random.seed(42)

    coordinates = []
    tracks = defaultdict(list)

    gps_traces_file_path = os.path.expanduser('~/gps_traces.csv')
    with open(gps_traces_file_path, 'r') as file:
        reader = csv.DictReader(file)
        for row in reader:
            coord = (float(row['Latitude']), float(row['Longitude']))
            coordinates.append(coord)
            tracks[row['TrackID']].append(coord)
    track_ids = list(tracks.keys())

    url = "http://localhost:5000"

    for _ in range(10000):
        start = random.choice(coordinates)
        end = random.choice(coordinates)
        
        start_coord = f"{start[1]:.6f},{start[0]:.6f}"
        end_coord = f"{end[1]:.6f},{end[0]:.6f}"

        response = requests.get(f"{url}/route/v1/driving/{start_coord};{end_coord}?overview=full&steps=true")
        if response.status_code != 200:
            raise Exception(f"Error: {response.status_code} {response.text}")



if __name__ == '__main__':
    main()