#!/bin/bash

while true; do
    # Perform a curl request to a specific URL
    curl -s -o /dev/null https://google.com  # Replace with your desired URL
    
    # Add a delay (e.g., 1 second) before the next request
    sleep 1
done
