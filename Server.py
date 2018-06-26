#!/usr/bin/env python

import socket
import struct
import sys
import binascii
import codecs
import json
import fcntl
import struct
import random
import paho.mqtt.subscribe as subscribe
import paho.mqtt.client as mqtt

import threading

from platform import system as system_name # get System OS name
from subprocess import call as system_call # execute as shell command


def sub_list_update():
    for ip in sensor_channels:
        for channel in sensor_channels[ip]:
            print('Subscribing the topic: ' + channel)
            client.subscribe(channel)
            
def pub_list_update(value):
    for ip in sensor_channels:
        for channel in actuator_channels[ip]:
            print('Publishing the topic: ' + channel)
            client.publish(channel, value)

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))

def on_message(client, userdata, msg):
    print(msg.topic + " " + str(msg.payload))
    

def ping(host):
    """
    Pings host string 
    """
    command = ['ping', '-c', '1', host]
    return system_call(command) == 0

def get_ip_address(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return socket.inet_ntoa(fcntl.ioctl(
        s.fileno(),
        0x8915,  # SIOCGIFADDR
        struct.pack('256s', bytes(ifname[:15], 'utf-8'))
    )[20:24])

def createAcknowledgement(sender_ip, receiver_ip, mqtt_port, in_topic, out_topic, ids, team_id):
    hostname = socket.gethostname()
    payload = json.dumps({'sender_ip': sender_ip, 'receiver_ip': receiver_ip, 'mqtt_port': mqtt_port,
                          'in_topic': in_topic, 'out_topic':out_topic, 'player_id': ids, 'team_id': team_id})
    return payload

def createInMessage(player_id, device_type, event, value):
    hostname = socket.gethostname()
    payload = json.dumps({'player_id': player_id, 'device_type': device_type, 'event': event, 'value': value})
    return payload

def createOutMessage(player_id, event, value):
    hostname = socket.gethostname()
    payload = json.dumps({'player_id': player_id, 'event': event, 'value': value})
    return payload


def validatePayload(payload):
    if 'ip_address' in payload and 'in_topic' in payload and 'out_topic' in payload:
        return True
    else:
        return False
"""
Example of out message
{
    "player_id": "1",
    "device_type": "target",
    "event": "hit",
    "value": "5"
  }
"""
def validateOutMessage(payload):
    if 'player_id' in payload and 'device_type' in payload and 'event' in payload and 'value' in payload:
        return True
    else:
        return False

"""
Example of out message
{
    "player_id": "1",
    "event": "hit",
    "value": "5"
  }
"""

def validateInMessage(payload):
    if 'player_id' in payload and 'event' in payload and 'value' in payload:
        return True
    else:
        return False
    

def UDPLoop():
    while(True):
        print('\nwaiting to receive message')
        data, address = sock.recvfrom(1024)

        print('received {} bytes from {}'.format(
            len(data), address))
        print('sending acknowledgement to', address)
        hexVal = binascii.b2a_hex(data)
        message = codecs.decode(hexVal, "hex")
        message = message.decode('utf-8')
        payload = json.loads(message)
        if validatePayload(payload):
            ip_address = payload['ip_address']
            #Team A has ID 0; Team B has ID 1
            if teamA>teamB:
                team_id = 1
                teamB++
            elif teamA<teamB:
                team_id = 0
                teamA++
            else:
                team_id = random.randint(0,1)
                if team_id == 0:
                    teamA++
                else:
                    teamB++
            ack = createAcknowledgement(myIP, ip_address, mqtt_port, in_topic, out_topic, ++ids, team_id)
            sock.sendto(str.encode(ack), address)
            #mosquitto_sub -h 172.20.10.3 -d -t hall_sensor

def MQTTLoop(client):
    while(True):
        client.loop()

multicast_group = '239.0.0.57'
server_address = ('', 5000)
in_topic = "in"
out_topic= "out"
ids = 0
teamA = 0
teamB = 0
# Create the socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# Bind to the server address
sock.bind(server_address)

# Tell the operating system to add the socket to
# the multicast group on all interfaces.
group = socket.inet_aton(multicast_group)
mreq = struct.pack('4sL', group, socket.INADDR_ANY)
sock.setsockopt(
    socket.IPPROTO_IP,
    socket.IP_ADD_MEMBERSHIP,
    mreq)
myIP = get_ip_address('wlan0')
# Receive/respond loop

# MQTT Connect
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883, 60)


thread1 = threading.Thread(target = UDPLoop)
thread1.deamon = True
thread1.start()
thread2 = threading.Thread(target = MQTTLoop, args=([client]))
thread2.deamon = True
thread2.start()   


