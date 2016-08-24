all: nanopb_pb2.py plugin_pb2.py

%_pb2.py: %.proto
	aprotoc --python_out=. $<
