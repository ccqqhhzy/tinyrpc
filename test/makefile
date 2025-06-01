
CC = g++
CPPFLAGS = -g -Wall -std=c++14 -I..
#INCLUDE = -I.

#####################
SRV = exe_server_test
OBJ = server_test.o \
	../buffer.o ../codec.o ../socket.o ../connection.o ../poller.o ../server.o \
	../util.o proto_pb/echo.pb.o proto_pb/hello.pb.o \
	../dispatcher_cc/protocol_cc.o ../dispatcher_cc/dispatcher.o \
	../dispatcher_pb/protocol_pb.o ../dispatcher_pb/dispatcher.o \
	../client/asyncall_poller.o ../client/client_pb.o
SRC = $(OBJ:.o=.cpp)

CLI = exe_client_pb_test
CLI_OBJ = client_pb_test.o \
	../buffer.o ../codec.o ../socket.o ../connection.o ../poller.o ../server.o \
	../util.o proto_pb/echo.pb.o proto_pb/hello.pb.o \
	../dispatcher_cc/protocol_cc.o ../dispatcher_cc/dispatcher.o \
	../dispatcher_pb/protocol_pb.o ../dispatcher_pb/dispatcher.o \
	../client/asyncall_poller.o ../client/client_pb.o
CLI_SRC = $(CLI_OBJ:.o=.cpp)

CC_CLI = exe_client_cc_test
CC_CLI_OBJ = client_cc_test.o \
	../buffer.o ../codec.o ../socket.o ../connection.o ../poller.o ../server.o \
	../util.o ../dispatcher_cc/protocol_cc.o ../dispatcher_cc/dispatcher.o \
	../dispatcher_pb/protocol_pb.o ../dispatcher_pb/dispatcher.o \
	../client/asyncall_poller.o ../client/client_cc.o

EXE_INCLUDE = -I/usr/local/include -I. -I.. -I./proto \

EXE_LOAD = -L/usr/local/bin -L/usr/bin -L .. -L ../business \
	-L/usr/local/bin/lib -L/usr/local/lib -lprotobuf -pthread

all: $(SRV) $(CLI) $(CC_CLI)
$(SRV):$(OBJ)
	$(CC) $(CPPFLAGS) $(EXE_INCLUDE) -o $@ $^ $(EXE_LOAD)
$(CLI):$(CLI_OBJ)
	$(CC) $(CPPFLAGS) $(EXE_INCLUDE) -o $@ $^ $(EXE_LOAD)
$(CC_CLI):$(CC_CLI_OBJ)
	$(CC) $(CPPFLAGS) $(EXE_INCLUDE) -o $@ $^ $(EXE_LOAD)

# node: sudo apt-get install libprotobuf-dev
# protoc --experimental_allow_proto3_optional --proto_path=./proto --cpp_out=./proto ./proto/echo.proto
protoc:
	protoc --proto_path=./proto_pb --cpp_out=./proto_pb ./proto_pb/echo.proto
	mv ./proto_pb/echo.pb.cc ./proto_pb/echo.pb.cpp
	protoc --proto_path=./proto_pb --cpp_out=./proto_pb ./proto_pb/hello.proto
	mv ./proto_pb/hello.pb.cc ./proto_pb/hello.pb.cpp

clean:
	rm -f $(SRV) $(CLI) $(CC_CLI)
	rm -f $(OBJ) $(CLI_OBJ) $(CC_CLI_OBJ)
