import json
from datetime import datetime
from kafka import KafkaConsumer, KafkaProducer

# pm2 start ./client_realtime_kafka_alarm.py --interpreter /usr/bin/python3

def alarm_rules(physicalentity, measurement, value, datetime):
    data = {
        'physicalentity': physicalentity,
        'datetime': datetime,
        'topic': 'Measurement out of range',
        'measurement': measurement,
        'value': value,
        'range': '[15-25] C'
    }
    alarm = False

    if measurement == 'T':
        if value < 15 or value > 25:
            data['range'] = '[15-25] C'
            alarm = True
    elif measurement == 'H':
        if value < 40 or value > 60:
            data['range'] = '[40-60] %'
            alarm = True
    elif measurement == 'S':
        if value > 80:
            data['range'] = '[0-80] dB'
            alarm = True
    elif measurement == 'M':
        if value > 250:
            data['range'] = '[0-250] ppm'
            alarm = True
    elif measurement == 'LI':
        if value < 400 or value > 600:
            data['range'] = '[400-600] lux'
            alarm = True

    if alarm:
        return data
    else:
        return None


consumer = KafkaConsumer('lumenconcept.telemetry', group_id='telemetry_alarm', bootstrap_servers=['ec2-18-204-96-185.compute-1.amazonaws.com:8089'])
producer = KafkaProducer(bootstrap_servers=['ec2-18-204-96-185.compute-1.amazonaws.com:8089'], value_serializer=lambda m: json.dumps(m).encode('utf-8'))

sensors = {}

for message in consumer:
    data = json.loads(message.value.decode('utf-8'))
    try:
        timestamp_current = datetime.strptime(data['datetime'], '%d-%m-%Y_%H:%M:%S')

        if data['physicalentity'] not in sensors:
            sensor_data = {
                'datetime': data['datetime'],
                'measurements': {}
            }
            for msmts in data['measurements']:
                sensor_data['measurements'][msmts['id']] = []
                sensor_data['measurements'][msmts['id']].append(msmts['value'])

            sensors[data['physicalentity']] = sensor_data
        else:
            for msmt in data['measurements']:
                if msmt['id'] not in sensor_data['measurements']:
                    sensor_data['measurements'][msmt['id']] = []
                sensor_data['measurements'][msmt['id']].append(msmt['value'])

        timestamp_start = datetime.strptime(sensors[data['physicalentity']]['datetime'], '%d-%m-%Y_%H:%M:%S')
        # print(timestamp_start)
        # print(timestamp_current)
        delta = timestamp_current - timestamp_start
        # print(delta.total_seconds())
        print(sensors)

        if delta.total_seconds() > 30: # CAMBIAR VENTANA DE TIEMPO EN SEGUNDOS
            for sensor_key, sensor_value in sensors.items():
                for msmts_key, msmts_value in sensor_value['measurements'].items():
                    count = 0
                    sum = 0
                    for msmt_value in msmts_value:
                        sum += float(msmt_value)
                        count = count + 1
                    avg = round(sum/count, 2)
                    print(msmts_key + ' = ' + str(avg))
                    alarm = alarm_rules(data['physicalentity'], msmts_key, avg, data['datetime'])
                    if alarm != None:
                        future = producer.send('lumenconcept.alarm', alarm)
                        try:
                            record_metadata = future.get(timeout=10)
                        except KafkaError:
                            log.exception()
                            pass
            sensors = {}
    except:
        pass
