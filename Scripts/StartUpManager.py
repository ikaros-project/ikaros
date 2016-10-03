# Python script to fire up ikaros using a browser and to check modules for errors
import BaseHTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
import sys
import base64
import os
import subprocess


key = ""

ikcFiles = []
descIkcFiles = [] # Not used
log = []
runningIkaros = False
# Test mode
cmd = ["../Bin/ikaros", "-s1"]
# Code from https://gist.github.com/fxsjy/5465353
class AuthHandler(SimpleHTTPRequestHandler):
    ''' Main class to present webpages and authentication. '''

    def do_IKAROS(self):
        #print "send header"
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        for val in make_html(ikcFiles):    
            self.wfile.write(val)

    def do_HEAD(self):
        #print "send header"
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_AUTHHEAD(self):
        #print "send header"
        self.send_response(401)
        self.send_header('WWW-Authenticate', 'Basic realm=\"Test\"')
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_GET(self):
        global key
        ''' Present frontpage with user authentication. '''
        if self.headers.getheader('Authorization') == None:
            self.do_AUTHHEAD()
            self.wfile.write('no auth header received')
            pass
        elif self.headers.getheader('Authorization') == 'Basic '+key:
            if (self.path == '/'):
                self.do_IKAROS()
            elif (self.path == '/ALL'):
                RunAll()
                self.do_IKAROS()
            else:
                RunIkaros(self.path[1:])
                self.do_IKAROS()
            pass
        else:
            self.do_AUTHHEAD()
            self.wfile.write(self.headers.getheader('Authorization'))
            self.wfile.write('not authenticated')
            pass

def test(HandlerClass = AuthHandler,
         ServerClass = BaseHTTPServer.HTTPServer):
    BaseHTTPServer.test(HandlerClass, ServerClass)

def findIKCFiles():
    print('Finding ikc files')
    for root, dirs, files in os.walk("../"):
        for file in files:
            if file.endswith(".ikc"):
                ikcFiles.append(os.path.join(root, file))
    return;

   

def make_html(data):
    yield '<html>'
    yield '<head>'
    yield '<style>'
    yield 'body {padding: 10px;}'
    yield '.ikcs {}'
    yield 'code {font-family: monospace;font-size: 12px;}'
    yield '.coding {background-color: lightgray; padding:10px; margin-top: 2px;margin-bottom: 12px;}'
    yield '.buttonRed {background-color: #FE0000; border: none; color: white;padding: 4px 20px ;text-align: center; text-decoration: none; display: inline-block; font-size: 14px; margin-right:8px;}'
    yield '.buttonYellow {background-color: #FEEF00; border: none; color: white;padding: 4px 20px ;text-align: center; text-decoration: none; display: inline-block; font-size: 14px;margin-right:8px;}'
    yield '.buttonGreen {background-color: #08FE00; border: none; color: white;padding: 4px 20px ;text-align: center; text-decoration: none; display: inline-block; font-size: 14px;margin-right:8px;}'
    yield '.buttonBlue {background-color: #87ABFF; border: none; color: white;padding: 4px 20px ;text-align: center; text-decoration: none; display: inline-block; font-size: 14px;margin-right:8px;}'
    yield '</style>'
    yield '<h1>Ikaros start up manager</h1>'
    yield '<h3>Run all ikc found:</h3>'
    yield '<a href= ALL>'
    yield '<button class="buttonBlue">Run All</button></a></br>'
    yield '<h3>Run ikc manually:</h3>'
    yield '<div class="ikcs">'
    for link, element in enumerate(data):
        yield '<a href="%s">' % link
        if (len(log) > link):
            if log[link] is None:
                yield '<button class="buttonBlue">Run</button></a>'
            elif log[link] is not None:
                if (log[link].find("WARNING") > 0):
                    yield '<button class="buttonYellow">Run</button></a>'
                elif (log[link].find("ERROR") > 0 or log[link].find("Error") > 0 ):
                    yield '<button class="buttonRed">Run</button></a>'
                else:
                    yield '<button class="buttonGreen">Run</button></a>'
            yield '%s' % element
        yield '</br>'
        if (len(log) > link):
            if log[link] is not None:
                if (log[link].find("WARNING") > 0):
                    yield '<div class="coding"><code>' 
                    yield "<br />".join(log[link].split("\n"))
                    yield '</code></div>' 
                elif (log[link].find("ERROR") > 0 or log[link].find("Error") > 0 ):
                    yield '<div class="coding"><code>' 
                    yield "<br />".join(log[link].split("\n"))
                    yield '</code></div>'
    yield '</div>'
    yield '</head>'
    yield '<body>'

# Fire up ikaros
def RunIkaros(arg):
    global log
    global arguments
    error = False
    runningIkaros = True
    
    try:
        t = subprocess.check_output(cmd + [ikcFiles[int(arg)],])
    except subprocess.CalledProcessError as grepexc:
        error = True
    if (error == True): 
        log.insert(int(arg), grepexc.output)
    else:
        del(log[int(arg)])
        log.insert(int(arg), t)

def RunAll():
    print('Running all ikc files:')
    for index, element in enumerate(ikcFiles):
        print(ikcFiles[index])
        RunIkaros(index)

def ParseIKC():
    print('Parsing ikc files')
    import xml.etree.ElementTree as ET
    for val in ikcFiles:    
        parsed = True
        try:
            tree = ET.parse(val)
        except ET.ParseError:
            print 'Can not parse ikc file:', val 
            parsed = False
            descIkcFiles.append("")
        if (parsed == True):
            root = tree.getroot()
            for desc in root.findall('description'):
                descIkcFiles.append(desc.text)
        else:
            descIkcFiles.append("")


    
if __name__ == '__main__':
    if len(sys.argv)<3:
        print "usage SimpleAuthServer.py [port] [username:password]"
        sys.exit()
    key = base64.b64encode(sys.argv[2])
    findIKCFiles()
    log = [None]*len(ikcFiles)
    ParseIKC()
    test()
