# Docker Orchestration

## Docker Images 
### osrm-backend-dev
Base image for telenav osrm-backend development, include all building and running dependencies.     
See details in [osrm-backend-dev docker](./osrm-backend-dev/).    

### osrm-backend
Image within built osrm binaries(`osrm-extract/osrm-partition/osrm-customize/...`) and running dependencies. It can be used to **compile data** or **startup routed**.      
See details in [osrm-backend docker](./osrm-backend/)

### osrm-frontend
Image contains web tool to check routing and guidance result.  
It uses MapBox GL JS and apply routing response on top of Mapbox vector tiles.  
See details in [osrm-frontend-docker](./osrm-frontend-docker/README.md)

