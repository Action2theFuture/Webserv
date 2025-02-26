#!/usr/bin/env python3
from datetime import datetime

print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>Current Time</h1>")
print(f"<p>The current time is: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>")
print("</body></html>")
