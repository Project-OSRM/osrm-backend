from locust import HttpUser, TaskSet, task, between

class OSRMTasks(TaskSet):
    @task
    def get_route(self):
        start = "13.388860,52.517037"
        end = "13.397634,52.529407"
        
        self.client.get(f"/route/v1/driving/{start};{end}?overview=full&steps=true")

class OSRMUser(HttpUser):
    tasks = [OSRMTasks]
    wait_time = between(1, 5)
