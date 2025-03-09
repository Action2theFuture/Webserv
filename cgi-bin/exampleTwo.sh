#!/bin/bash

# Get PATH_INFO
path_info=$PATH_INFO
echo "PATH_INFO = $path_info"

if [ "$path_info" = "/env" ]; then
    # Print environment variables
    echo "Content-Type: text/html"
    echo ""
    echo "<html><body>"
    echo "<h1>Environment Variables:</h1>"
    echo "<pre>"
    printenv
    echo "</pre>"
    echo "</body></html>"
else
    # Print a simple message
    echo "Content-Type: text/html"
    echo ""
    echo "<html><body>"
    echo "<h1>Thanks for visiting!</h1>"
    echo "</body></html>"
fi
