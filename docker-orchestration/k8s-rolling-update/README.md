# Kubernetes Rolling Update Deployment
Use kubernetes rolling update deployment strategy for timed replace container with new one. Latest traffic will be used during container startup.  

## Usage
```bash

# NOTE: prepare your image and fill into osrm.yaml
$ sed -i "s#TELENAV_OSRM_BACKEND_DOCKER_IMAGE#<your image>#g" ./osrm-backend.yaml

# create deployment
$ kubectl create -f osrm-backend.yaml  
deployment.extensions/osrm-backend created

# create loadbalancer
$ kubectl create -f service.yaml  
service/routing-osrm-service created

# OPTIONAL: checking deployment status, wait READY
$ kubectl get deployments -n routing-osrm
NAME   READY   UP-TO-DATE   AVAILABLE   AGE
osrm-backend   0/1     1            0           20s
$ kubectl get pods -n routing-osrm
NAME                    READY   STATUS    RESTARTS   AGE
osrm-backend-67b55f46c9-tpfgs   0/1     Running   0          22s

# OPTIONAL: check deployment details for troubleshooting
$ kubectl describe deployments -n routing-osrm
...
$ kubectl describe pods -n routing-osrm
...

# OPTIONAL: check container running log for troubleshooting
$ kubectl logs -f --timestamps=true  -n routing-osrm osrm-backend-67b55f46c9-tpfgs
...

# start timed rolling update for traffic updating
$ crontab osrm-backend-cronjob
$ crontab -l 
*/20 * * * * export PATH=$HOME/bin:$PATH && kubectl set env --env="LAST_MANUAL_RESTART=$(date -u +\%Y\%m\%dT\%H\%M\%S)" deploy/osrm-backend -n routing-osrm >> ${HOME}/osrm-backend-cronjob.log 2>&1

# OPTIONAL: if want to stop
$ crontab -r 
$ kubectl delete deployment osrm-backend -n routing-osrm
deployment.extensions "osrm-backend" deleted
$ kubectl delete routing-osrm-service -n routing-osrm
service "routing-osrm-service" deleted

```