import os


# Get PATH_INFO

path_info = os.environ.get('PATH_INFO')

if (path_info == '/env' or path_info == '/env/'):
    # Print environment variables
    print("Content-Type: text/html\n")
    print("<html><body>")
    print("<h1>Environment Variables:</h1>")
    print("<pre>")

    for key, value in os.environ.items():
        print(f"{key}: {value}")

    print("</pre>")
    print("</body></html>")


else:
    # Print a simple message
    print("Content-Type: text/html\n")
    print("<html><body>")
    print("<h1>Welcome to our site!</h1>")
    print("</body></html>")