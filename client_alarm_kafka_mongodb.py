import json
from kafka import KafkaConsumer
from pymongo import MongoClient

# pm2 start ./client_realtime_kafka_mongodb.py --interpreter /usr/bin/python3

consumer = KafkaConsumer('lumenconcept.alarm', group_id='alarm_persistence', bootstrap_servers=['ec2-18-204-96-185.compute-1.amazonaws.com:8089'])
client = MongoClient('ec2-34-202-239-178.compute-1.amazonaws.com', 8087)
db = client['lumenconcept_telemetry']

for message in consumer:
    data = json.loads(message.value.decode('utf-8'))
    db.alarm.insert_one(data)
