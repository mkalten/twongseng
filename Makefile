
SRC=main.cpp \
	twongseng.cpp \
	TUIO2/TuioServer.cpp \
	TUIO2/TuioTime.cpp \
	TUIO2/TuioManager.cpp \
	TUIO2/TuioComponent.cpp \
	TUIO2/TuioObject.cpp \
	TUIO2/TuioToken.cpp \
	TUIO2/TuioSymbol.cpp \
	TUIO2/TuioPointer.cpp \
	TUIO2/TuioBounds.cpp \
	TUIO2/TuioPoint.cpp \
	TUIO2/TuioDispatcher.cpp \
	TUIO2/UdpSender.cpp \
	TUIO2/TcpSender.cpp \
	TUIO2/WebSockSender.cpp \
	oscpack/ip/posix/NetworkingUtils.cpp \
	oscpack/ip/posix/UdpSocket.cpp \
	oscpack/osc/OscOutboundPacketStream.cpp \
	oscpack/osc/OscTypes.cpp

OBJS=$(SRC:.cpp=.o)
CPPFLAGS=-ITUIO2 -Ioscpack
LIBS=-F/System/Library/PrivateFrameworks -framework MultitouchSupport -framework CoreFoundation
BIN=twongseng

all : $(BIN)

.cpp.o :
	g++ $(CPPFLAGS) -c $< -o $@

$(BIN) : $(OBJS)
	g++ -o $@ $(LIBS) $^

clean :
	rm -f $(BIN) $(OBJS)

