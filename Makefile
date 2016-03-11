INCLUDE = -I/opt/software/mysql-connector-c-commercial-6.1.3-linux-glibc2.5-x86_64/include \
		  -I/opt/software/openssl-1.0.1e/include \
          -I/opt/software/boost_1_59_0

LIBS = -lpthread \
	   -lmysqlclient_r \
	   -lsqlite3 \
	   -lrt \
	   -lboost_random \
	   -lboost_regex \
       -lboost_chrono \
	   -lboost_timer \
	   -lboost_system \
	   -lboost_thread \
	   -lboost_context \
	   -lboost_coroutine \
       -lboost_unit_test_framework
	  
LIBPATH = -L/opt/software/boost_1_59_0/stage/lib \
		  -L/opt/mysql-connector-c-commercial-6.1.3-linux-glibc2.5-x86_64/lib

OBJS =  bingo_random.o \
		bingo_mq_service.o \
		bingo_security.o \
		bingo_node.o \
		bingo_log.o \
		bingo_persistent_mysql.o \
		bingo_persistent_sqlite.o \
		bingo_process_task_shm_req_and_rep.o \
		bingo_process_task_shm_req.o \
		bingo_process_task_ask.o \
		bingo_mem_guard.o \
		bingo_thread_task.o \
		bingo_atomic_lock.o \
		bingo_string.o \
		bingo_singleton.o \
		bingo_tcp_server.o \
		bingo_tcp_client.o \
		main.o \
		bingo/mq/mq_client.o \
		bingo/mq/mq_client_hgr.o \
		bingo/mq/mq_package.o \
		bingo/mq/mq_server.o \
		bingo/mq/mq_server_hgr.o \
		bingo/security/all.o \
		bingo/cfg/xml/parse_handler.o \
		bingo/cfg/json/parse_handler.o \
		bingo/persistent/mysql/connector.o \
		bingo/persistent/db_connector.o \
		bingo/tss.o
		
CPPS =  bingo/mq/mq_client.cpp \
		bingo/mq/mq_client_hgr.cpp \
		bingo/mq/mq_package.cpp \
		bingo/mq/mq_server.cpp \
		bingo/mq/mq_server_hgr.cpp \
		bingo/security/all.cpp \
		bingo/cfg/xml/parse_handler.cpp \
		bingo/cfg/json/parse_handler.cpp \
		bingo/persistent/mysql/connector.cpp \
		bingo/persistent/db_connector.cpp \
		bingo/tss.cpp
		
ifeq ($(ShowDebug),y)
DEBUGS = -DBINGO_MQ_DEBUG
else
DEBUGS =
endif

ifeq ($(findstring Exe_Debug,$(ConfigName)),Exe_Debug)
	CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 $(DEBUGS) $(INCLUDE) 
	TARGET = test
else ifeq ($(findstring Exe_Release,$(ConfigName)),Exe_Release)
	CXXFLAGS =	-O2 -Wall -fmessage-length=0 $(INCLUDE)
	TARGET = test
else ifeq ($(findstring Lib_Debug,$(ConfigName)),Lib_Debug)
	CXXFLAGS =	-O2 -g -fPIC  -shared $(INCLUDE)
	TARGET = libbingo.so
else ifeq ($(findstring Lib_Release,$(ConfigName)),Lib_Release)
	CXXFLAGS =	-O2 -fPIC  -shared $(INCLUDE)
	TARGET = libbingo.so
endif

ifeq ($(findstring Exe_,$(ConfigName)),Exe_)
$(TARGET):	$(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS) $(LIBPATH);

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
else ifeq ($(findstring Lib_,$(ConfigName)),Lib_)
all:
	$(CXX)  $(CXXFLAGS) -o $(TARGET) $(CPPS) $(LIBS) $(LIBPATH);

clean:
	rm -f $(TARGET)
endif


