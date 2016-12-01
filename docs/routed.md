## Environment Variables

### SIGNAL_PARENT_WHEN_READY

If the SIGNAL_PARENT_WHEN_READY environment variable is set osrm-routed will
send the USR1 signal to its parent when it will be running and waiting for
requests. This could be used to upgrade osrm-routed to a new binary on the fly
without any service downtime - no incoming requests will be lost.

### DISABLE_ACCESS_LOGGING

If the DISABLE_ACCESS_LOGGING environment variable is set osrm-routed will
**not** log any http requests to standard output. This can be useful in high
traffic setup.
