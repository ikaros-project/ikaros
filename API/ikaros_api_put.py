#!/usr/bin/env python3.11
"""
Example python script to talk to the Ikaros API by sending a JSON data structure to a module using 'command'

The code uses the requests library for Python. Intall it using:

> pip install requests

"""

import requests
import json

# Define the URL to which the PUT request will be sent
# The command is sent to the root group 'Untitled' tow ork with empty network
# Use full path to module, i.e. 'A.B.C' to access that module

url = 'http://127.0.0.1:8000/command/Untitled/'

# Define the JSON data to be sent in the body of the PUT request

data = {
    'command': "do_something",          # The command value is mandatory"
    'x': '42',                          # The rest of the data is optional and can be anything the module recognizes
    'text': 'my message to the module'  # The data is received in the Ikaros module as a dictionary
}

json_data = json.dumps(data)
headers = {
    'Content-Type': 'application/json'
}

try:
    # Send the PUT request with the JSON

    response = requests.put(url, headers=headers, data=json_data)

    # Check if the request was successful
    if response.status_code == 200:
        print('Request was successful.')
        print('Response:', response.json()) # Response message that can generally be ignored
    else:
        print(f'Request failed with status code: {response.status_code}')
        print('Response:', response.text)

except requests.exceptions.RequestException as e:
    print(f'An error occurred: {e}')