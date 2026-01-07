## Environment Variables

### SHM_LOCK_DIR

If the SHM_LOCK_DIR environment variable is set, OSRM will use it as the
directory for shared memory lock files instead of the system temporary directory.
This is useful in containerized environments (Docker/Kubernetes) where the lock
file directory should persist across container restarts when loading from shared
memory.

### SIGNAL_PARENT_WHEN_READY

If the SIGNAL_PARENT_WHEN_READY environment variable is set osrm-routed will
send the USR1 signal to its parent when it will be running and waiting for
requests. This could be used to upgrade osrm-routed to a new binary on the fly
without any service downtime - no incoming requests will be lost.

### DISABLE_ACCESS_LOGGING

If the DISABLE_ACCESS_LOGGING environment variable is set osrm-routed will
**not** log any http requests to standard output. This can be useful in high
traffic setup.
