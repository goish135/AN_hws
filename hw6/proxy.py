import zmq

context = zmq.Context()

frontend = context.socket(zmq.ROUTER)
backend = context.socket(zmq.DEALER)


frontend.bind("tcp://127.0.0.1:5561")
backend.bind("tcp://127.0.0.1:5562")

zmq.proxy(frontend, backend)


frontend.close()
backend.close()
context.term()
