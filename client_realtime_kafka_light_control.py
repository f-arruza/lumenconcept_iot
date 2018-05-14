import json
import paho.mqtt.client as mqtt
from datetime import datetime
from kafka import KafkaConsumer

# pm2 start ./client_realtime_kafka_light_control.py --interpreter /usr/bin/python3

client = mqtt.Client()
client.connect('ec2-18-204-96-185.compute-1.amazonaws.com', 8083, 60)
client.loop_start()

consumer = KafkaConsumer('lumenconcept.telemetry', group_id='telemetry_light_control', bootstrap_servers=['ec2-18-204-96-185.compute-1.amazonaws.com:8089'])

threshold = 500
point_light = -1
sensors = {}

for message in consumer:
    if point_light == -1:
        point_light = 250
        client.publish('lumenconcept.telemetry.lumeniot001',
                       'FADE:' + str(point_light), qos=2)

    data = json.loads(message.value.decode('utf-8'))
    try:
        timestamp_current = datetime.strptime(data['datetime'], '%d-%m-%Y_%H:%M:%S')

        if data['physicalentity'] not in sensors:
            sensor_data = {
                'datetime': data['datetime'],
                'measurements': {}
            }
            for msmt in data['measurements']:
                if msmt['id'].startswith('LI'):
                    sensor_data['measurements'][msmt['id']] = []
                    sensor_data['measurements'][msmt['id']].append(msmt['value'])

            sensors[data['physicalentity']] = sensor_data
        else:
            for msmt in data['measurements']:
                if msmt['id'].startswith('LI'):
                    if msmt['id'] not in sensor_data['measurements']:
                        sensor_data['measurements'][msmt['id']] = []
                    sensor_data['measurements'][msmt['id']].append(msmt['value'])

        timestamp_start = datetime.strptime(sensors[data['physicalentity']]['datetime'], '%d-%m-%Y_%H:%M:%S')
        delta = timestamp_current - timestamp_start
        print(delta.total_seconds())
        print(sensors)

        if delta.total_seconds() > 30: # CAMBIAR VENTANA DE TIEMPO EN SEGUNDOS
            for sensor_key, sensor_value in sensors.items():
                for msmts_key, msmts_value in sensor_value['measurements'].items():
                    count = 0
                    sum = 0
                    for msmt_value in msmts_value:
                        sum += float(msmt_value)
                        count += 1
                    avg = round(sum/count, 2) # Luz actual

                    diff = threshold - avg    # Diferencia en (LUX) con el umbral
                    point_light = point_light + int(diff / 2)

                    if point_light < 0:
                        point_light = 0;
                    if point_light > 255:
                        point_light = 255;

                    client.publish('lumenconcept.telemetry.lumeniot001',
                                   'FADE:' + str(point_light), qos=2)

                    print('Luz actual = ' + str(avg))
                    print('Diferencia lux = ' + str(diff))
                    print('Puntos actuales = ' + str(point_light))

            sensors = {}
    except:
        pass
