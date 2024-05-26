from locust import HttpUser, TaskSet, task, between

class OSRMTasks(TaskSet):
    @task
    def get_route(self):
        start = "13.388860,52.517037"
        end = "13.397634,52.529407"
        
        self.client.get(f"/route/v1/driving/{start};{end}?overview=full&steps=true")

    @task
    def get_table(self):
        coordinates = "13.388860,52.517037;13.397634,52.529407;13.428555,52.523219"
        self.client.get(f"/table/v1/driving/{coordinates}")

class OSRMUser(HttpUser):
    tasks = [OSRMTasks]
    wait_time = between(1, 5)
