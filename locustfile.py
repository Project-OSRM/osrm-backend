from locust import HttpUser, TaskSet, task, between

class OSRMTasks(TaskSet):
    @task
    def get_route(self):
        # Define the coordinates for the route
        start = "13.388860,52.517037"
        end = "13.397634,52.529407"
        
        # Make a request to the OSRM route service
        self.client.get(f"/route/v1/driving/{start};{end}?overview=false")

class WebsiteUser(HttpUser):
    tasks = [OSRMTasks]
    wait_time = between(1, 5)
