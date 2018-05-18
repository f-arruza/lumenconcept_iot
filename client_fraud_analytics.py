import json
from kafka import KafkaConsumer
from pymongo import MongoClient

# pm2 start ./client_fraud_analytics.py --interpreter /usr/bin/python3

consumer = KafkaConsumer('lumenconcept.fraudAlarm',
                         group_id='fraud_persistence',
                         bootstrap_servers=['ec2-18-204-96-185.compute-1.amazonaws.com:8089'])
client = MongoClient('ec2-34-202-239-178.compute-1.amazonaws.com', 8087)
db = client['lumenconcept_speed_tier']

for message in consumer:
    data = json.loads(message.value.decode('utf-8'))
    db.fraud.insert_one(data)
