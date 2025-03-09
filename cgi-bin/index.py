import os

print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>Index . py!</h1>")
print("<p>Default CGI script.</p>")

path_info = os.environ.get('PATH_INFO')

if path_info:
    print(f"<p>PATH_INFO: {path_info}</p>")

print("</body></html>")