#!/usr/bin/env python3
import os

print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>Environment Variables</h1>")
for key, value in os.environ.items():
    print(f"<p><strong>{key}:</strong> {value}</p>")
print("</body></html>")
