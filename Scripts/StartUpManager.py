# Python script to fire up ikaros using a browser and to check modules for errors
import BaseHTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
import sys
import base64
import os
import subprocess

# Files that will not work with the script
excludeFiles = ["NetworkCamera_test.ikg", "FadeCandy_test.ikg", "Phidgets_test.ikg","IPCServer_test.ikg","IPCClient_test.ikg"]
searchPattern = "_test.ikg"

key = ""

ikgFiles = []
descIkgFiles = [] # Not used
log = []
runningIkaros = False
# Test mode
cmd = ["../Bin/ikaros", "-s1", "-R"]
# Code from https://gist.github.com/fxsjy/5465353
class AuthHandler(SimpleHTTPRequestHandler):
    ''' Main class to present webpages and authentication. '''

    def do_IKAROS(self):
        #print "send header"
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        for val in make_html(ikgFiles):    
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

def findIKGFiles():
    print('Finding ikg files')
    print('Exluded files (edit script to change this):')
    for e in excludeFiles:
        print(e)
    exlude = False

    # for root, dirs, files in os.walk("../Demos"):
    #     for file in files:
    #         if file.endswith(".ikg"):
    #             exlude = False
    #             for e in excludeFiles:
    #                 if file.endswith(e):
    #                     exlude = True
    #             if (not exlude):
    #                 ikgFiles.append(os.path.join(root, file))
    # for root, dirs, files in os.walk("../Examples"):
    #     for file in files:
    #         if file.endswith(".ikg"):
    #             exlude = False
    #             for e in excludeFiles:
    #                 if file.endswith(e):
    #                     exlude = True
    #             if (not exlude):
    #                 ikgFiles.append(os.path.join(root, file))
    for root, dirs, files in os.walk("../"):
        for file in files:
            if file.endswith(searchPattern):
                exlude = False
                for e in excludeFiles:
                    print e  + " matches " + file

                    if file.endswith(e):
                        exlude = True
                if (not exlude):
                    ikgFiles.append(os.path.join(root, file))
    return

   

def make_html(data):
    yield '<html>'
    yield '<head>'
    yield '<style>'
    yield 'body {padding: 10px;}'
    yield '.ikgs {}'
    yield 'code {font-family: monospace;font-size: 12px;}'
    yield '.coding {background-color: lightgray; padding:10px; margin-top: 2px;margin-bottom: 12px;}'
    yield '.buttonRed {background-color: #FE0000; border: none; color: white;padding: 4px 20px ;text-align: center; text-decoration: none; display: inline-block; font-size: 14px; margin-right:8px;}'
    yield '.buttonYellow {background-color: #FEEF00; border: none; color: white;padding: 4px 20px ;text-align: center; text-decoration: none; display: inline-block; font-size: 14px;margin-right:8px;}'
    yield '.buttonGreen {background-color: #08FE00; border: none; color: white;padding: 4px 20px ;text-align: center; text-decoration: none; display: inline-block; font-size: 14px;margin-right:8px;}'
    yield '.buttonBlue {background-color: #87ABFF; border: none; color: white;padding: 4px 20px ;text-align: center; text-decoration: none; display: inline-block; font-size: 14px;margin-right:8px;}'
    yield '</style>'
    yield '<h1>Ikaros start up manager</h1>'
    yield '<h3>Run all ikg found:</h3>'
    yield '<a href= ALL>'
    yield '<button class="buttonBlue">Run All</button></a></br>'
    yield '<h3>Run ikg manually:</h3>'
    yield '<div class="ikgs">'
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
        t = subprocess.check_output(cmd + [ikgFiles[int(arg)],])
    except subprocess.CalledProcessError as grepexc:
        error = True
    if (error == True): 
        log.insert(int(arg), grepexc.output)
    else:
        del(log[int(arg)])
        log.insert(int(arg), t)

def RunAll():
    print('Running all ikg files:')
    for index, element in enumerate(ikgFiles):
        print(ikgFiles[index])
        RunIkaros(index)

def ParseIKG():
    print('Parsing ikg files')
    import xml.etree.ElementTree as ET
    for val in ikgFiles:    
        parsed = True
        try:
            tree = ET.parse(val)
        except ET.ParseError:
            print 'Can not parse ikg file:', val 
            parsed = False
            descIkgFiles.append("")
        if (parsed == True):
            root = tree.getroot()
            for desc in root.findall('description'):
                descIkgFiles.append(desc.text)
        else:
            descIkgFiles.append("")
    
if __name__ == '__main__':
    if os.geteuid() != 0:
        exit("You need to have root privileges to run this script.\n")
    if len(sys.argv)<3:
        print "usage SimpleAuthServer.py [port] [username:password] [optional search pattern]"
        sys.exit()
    if len(sys.argv)==4:
        searchPattern = sys.argv[3]
        print "Seaching must end with"
        print searchPattern
    # Check for compiled ikaros
    import os.path
    if (os.path.isfile(cmd[0]) == True):
        print "Found ikaros binary"
    else:
        print cmd[0] + " not found. Compile ikaros."
        exit(-1)

    key = base64.b64encode(sys.argv[2])
    findIKGFiles()
    log = [None]*len(ikgFiles)
    ParseIKG()
    test()
