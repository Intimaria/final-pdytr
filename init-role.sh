#!/bin/bash

#!/bin/bash

# Define roles
roles=(
    "websocket-client-server"
    "synchronization-setup"
    "prometheus"
    "grafana"
    "docker-compose"
    "metrics-analysis"
)

# Initialize each role
for role in "${roles[@]}"; do
    echo "Initializing role: $role"
    ansible-galaxy init "roles/$role"
done

