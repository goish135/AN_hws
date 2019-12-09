import zmq
import time

context = zmq.Context()

sender = context.socket(zmq.PUB)
sender.connect("tcp://127.0.0.1:5559")

controller = context.socket(zmq.REQ)
controller.connect("tcp://127.0.0.1:5561")

# one hand shake 
# send reqest 'Hello'
controller.send(b'Hello')
# receive 'yes' then send formal data
rep = controller.recv()
print('rep = ', rep)
if rep == b'yes':
    for i in range(3):
        print(i," formal data")
        sender.send(b'formal data')
else:
    print("Some error")
