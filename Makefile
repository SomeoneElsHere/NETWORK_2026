# Compilation options
COMPILE = g++ -c
LINK = g++ -o

NAME_CLIENT = p2p_client
NAME_SERVER = p2p_server

SUBDIR_SRC = src
SUBDIR_SRC_CLIENT = ./$(SUBDIR_SRC)/$(NAME_CLIENT)
SUBDIR_SRC_SERVER = ./$(SUBDIR_SRC)/$(NAME_SERVER)

SUBDIR_BUILD = build
SUBDIR_BUILD_CLIENT = ./$(SUBDIR_BUILD)/$(NAME_CLIENT)
SUBDIR_BUILD_SERVER = ./$(SUBDIR_BUILD)/$(NAME_SERVER)

# -- Shared code compilation
# TODO: Add any code shared between the client and server here
# SHARED_OBJS = 

# -- Client compilation
CLIENT_OBJS = $(SUBDIR_BUILD_CLIENT)/peer.o
CLIENT_EXECUTABLE = $(SUBDIR_BUILD_CLIENT)/$(NAME_CLIENT)

$(CLIENT_EXECUTABLE): $(CLIENT_OBJS)
	$(LINK) $(CLIENT_EXECUTABLE) $(CLIENT_OBJS)

$(SUBDIR_BUILD_CLIENT)/peer.o: $(SUBDIR_SRC_CLIENT)/peer.cpp $(SUBDIR_SRC_CLIENT)/peer.h
	$(COMPILE) $(SUBDIR_SRC_CLIENT)/peer.cpp -o $(SUBDIR_BUILD_CLIENT)/peer.o

# -- Server compilation
# TODO: Begin writing tracker server program
# SERVER_OBJS = $(SUBDIR_BUILD_SERVER)/server.o
# SERVER_EXECUTABLE = $(SUBDIR_BUILD_SERVER)/$(NAME_SERVER)

# $(SERVER_EXECUTABLE): $(SERVER_OBJS)
#     $(LINK) $(SERVER_EXECUTABLE) $(SERVER_OBJS)

# Remove compiled object files
clean:
	rm -rf $(SUBDIR_BUILD)/*.o
	rm -f $(SUBDIR_BUILD_CLIENT)/
	rm -f $(SUBDIR_BUILD_SERVER)/