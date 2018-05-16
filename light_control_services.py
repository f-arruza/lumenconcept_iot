from flask import Flask, Response, json
from flask_cors import CORS
from pymongo import MongoClient

import paho.mqtt.client as mqtt


app = Flask(__name__)
cors = CORS(app, resources={r"/lights/*": {"origins": "*"}})

client = mqtt.Client()
client.connect('ec2-18-204-96-185.compute-1.amazonaws.com', 8083, 60)
client.loop_start()

mongo_client = MongoClient('ec2-34-202-239-178.compute-1.amazonaws.com', 8087)
db = mongo_client['lumenconcept_telemetry']

@app.route('/lights/<string:command>/')
def execute_command(command):
    if command != 'ON' and command != 'OFF':
        data = {
            'success': 'false',
            'detail': 'Wrong command.'
        }
    else:
        msg = 'LIGHT_ON:0'
        if command == 'OFF':
            msg = 'LIGHT_OFF:0'
        client.publish('lumenconcept.telemetry.lumeniot001',
                       msg, qos=2)
        data = {
            'success': 'true'
        }
    return  Response(json.dumps(data), mimetype=u'application/json',
                     status=200)

@app.route('/lights/fade/<int:value>/')
def set_fade(value):
    msg = 'FADE:' + str(value)
    if value <= 0:
        value = 0
        msg = 'LIGHT_OFF:0'
    elif value >= 255:
        value = 255
        msg = 'LIGHT_ON:0'

    client.publish('lumenconcept.telemetry.lumeniot001',
                   msg, qos=2)
    data = {
        'success': 'true'
    }
    return  Response(json.dumps(data), mimetype=u'application/json',
                     status=200)

@app.route('/lights/mode/<string:mode>/')
def set_mode(mode):
    if mode != 'A' and mode != 'M':
        data = {
            'success': 'false',
            'detail': 'Wrong mode.'
        }
    else:
        db.parameters.find_one_and_update({"physicalentity": "LC01"}, {'$set': {'mode': mode}})
        data = {
            'success': 'true'
        }
    return  Response(json.dumps(data), mimetype=u'application/json',
                     status=200)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080, debug=True)
