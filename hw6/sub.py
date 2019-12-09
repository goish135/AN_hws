import zmq

context = zmq.Context()

receiver = context.socket(zmq.SUB)

# Subscribe any topic
receiver.setsockopt(zmq.SUBSCRIBE, b'')
receiver.connect("tcp://127.0.0.1:5560")

controller = context.socket(zmq.REP)
controller.connect("tcp://127.0.0.1:5562")

# one hand shake , to confirm data transfer integrity
req = controller.recv()
print('req = ', req)
if req == b'Hello':
    controller.send(b'yes')
    while True:
        msg = receiver.recv_multipart()
        print('Subscriber got ', msg)
else:
    print('Some error')
    
