from http.server import HTTPServer, SimpleHTTPRequestHandler
import os

class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=os.path.dirname(os.path.realpath(__file__)), **kwargs)

httpd = HTTPServer(('localhost', 0), Handler)

print("Go to http://localhost:{0}/bin/game.html".format(httpd.server_port))

httpd.serve_forever()
