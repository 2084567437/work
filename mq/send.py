#!/usr/bin/env python  
import pika  
rabbit_username='zhaoj'
rabbit_password='zhaoj'
credentials = pika.PlainCredentials(rabbit_username, rabbit_password)
connection = pika.BlockingConnection(pika.ConnectionParameters( host='192.168.64.131',credentials=credentials))
#print connetion
#connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))  
channel = connection.channel()  

channel.queue_declare(queue='mq-queue-pushtask-0')  

channel.basic_publish(exchange='vip-mq-ex-seg_pushtask',  routing_key='vip-mq-rkey-pushtask-0',   body='Hello World!')  
print " [x] Sent 'Hello World!'"  
connection.close()  
