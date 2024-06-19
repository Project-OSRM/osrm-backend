import requests
import xml.etree.ElementTree as ET
import csv
import sys
import argparse

def get_osm_gps_traces(bboxes):
    url = 'https://api.openstreetmap.org/api/0.6/trackpoints'
    traces = []
    
    lon_step = 0.25
    lat_step = 0.25

    for bbox in bboxes:
        min_lon, min_lat, max_lon, max_lat = map(float, bbox.split(','))
        
        current_min_lon = min_lon
        while current_min_lon < max_lon:
            current_max_lon = min(current_min_lon + lon_step, max_lon)
            
            current_min_lat = min_lat
            while current_min_lat < max_lat:
                current_max_lat = min(current_min_lat + lat_step, max_lat)
                
                bbox_str = f'{current_min_lon},{current_min_lat},{current_max_lon},{current_max_lat}'
                print(f"Requesting bbox: {bbox_str}", file=sys.stderr)
                
                params = {
                    'bbox': bbox_str,
                    'page': 0
                }
                headers = {
                    'Accept': 'application/xml'
                }
                
                response = requests.get(url, params=params, headers=headers)
                if response.status_code == 200:
                    traces.append(response.content)
                else:
                    print(f"Error fetching data for bbox {bbox_str}: {response.status_code} {response.text}", file=sys.stderr)
                
                current_min_lat += lat_step
            current_min_lon += lon_step
    
    return traces

def parse_gpx_data(gpx_data):
    try:
        root = ET.fromstring(gpx_data)
    except ET.ParseError as e:
        print(f"Error parsing GPX data: {e}", file=sys.stderr)
        return []
    namespace = {'gpx': 'http://www.topografix.com/GPX/1/0'}

    tracks = []
    for trk in root.findall('.//gpx:trk', namespace):
        track_data = []
        for trkseg in trk.findall('.//gpx:trkseg', namespace):
            for trkpt in trkseg.findall('gpx:trkpt', namespace):
                lat = trkpt.get('lat')
                lon = trkpt.get('lon')
                time = trkpt.find('time').text if trkpt.find('time') is not None else ''
                track_data.append([lat, lon, time])
        tracks.append(track_data)
    return tracks

def save_to_csv(data, file):
    writer = csv.writer(file)
    writer.writerow(['TrackID', 'Latitude', 'Longitude', 'Time'])
    writer.writerows(data)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Fetch and output OSM GPS traces for given bounding boxes.')
    parser.add_argument('bboxes', nargs='+', help='Bounding boxes in the format min_lon,min_lat,max_lon,max_lat')
    
    args = parser.parse_args()
    
    gpx_data_traces = get_osm_gps_traces(args.bboxes)
    print(f"Collected {len(gpx_data_traces)} trace segments", file=sys.stderr)

    all_data = []
    track_id = 0
    for gpx_data in gpx_data_traces:
        for track in parse_gpx_data(gpx_data):
            for point in track:
                all_data.append([track_id] + point)
            track_id += 1
    
    # Output all data to stdout
    save_to_csv(all_data, sys.stdout)
