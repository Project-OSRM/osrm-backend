# Docker Compose Setup for Project OSRM

This directory contains a **Docker Compose configuration** to simplify running [Project OSRM](https://github.com/Project-OSRM/osrm-backend).

With this way:

* Only use docker compose up -d  
* It launches a **frontend service** with health checks.
* Don't need to ensure services start in the correct order.

---

##  Directory Structure

```
osrm-backend/
├── docker-compose.yml
├── prepare_osrm.sh
```

---

##  Quick Start

###  Clone this repository

```bash
git clone https://github.com/elshan2000/osrm-backend.git
cd osrm-backend
```

###

```
### How It Works
```

### Init Service

* Checks for the marker file `osrm_is_created`.
* If not present, runs:

  * `osrm-extract`
  * `osrm-partition`
  * `osrm-customize`
* Creates the marker file.
* Exits after preparation.

### Backend Service

* Depends on the **frontend healthcheck**.
* Starts `osrm-routed` serving prepared data.

### Frontend Service

* Periodically checks for the presence of the marker file `osrm_is_created`.
* Reports `healthy` when the preparation is complete.
* This readiness allows the backend service to start safely.

---

##  Health Check Logic

The **frontend** container performs a health check by verifying:

```
/data/osrm_is_created
```

Once the file exists, the health check reports `healthy`. This ensures that:

 Data preparation is complete before routing starts.

---

##  Clean Up

To stop and remove all containers:

```bash
docker compose down
```

To force re-preparation of the data (for example, if you want to regenerate `.osrm` files):

```bash
docker compose down
rm ./data/osrm_is_created
docker compose up -d
```

---

##  Contribution

This Docker Compose configuration was contributed by [elshan2000](https://github.com/elshan2000).

Feel free to open issues or submit improvements!

---

##  License

This project follows the [Project OSRM](https://github.com/Project-OSRM/osrm-backend) license (BSD-2-Clause).
