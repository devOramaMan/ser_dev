_AM_CFLAGS = $(EXTRA_CFLAGS) -I$(top_srcdir)/CDM_v2.12.36.4_WHQL_Certified
LDADD = $(EXTRA_LDFLAGS) -L$(top_srcdir)/CDM_v2.12.36.4_WHQL_Certified/amd64/ -lftd2xx$(HOST_OS_FLAG) -lzmq 
LIB = 

if DEBUG
    AM_CFLAGS = $(_AM_CFLAGS) -g3 -O0
    AM_CXXFLAGS = $(_AM_CFLAGS) -g3 -O0
else
    AM_CFLAGS = $(_AM_CFLAGS) -O2 -s
    AM_CXXFLAGS = $(_AM_CFLAGS) -O2 -s
endif

bin_PROGRAMS = ser_ftdi

ser_ftdi_SOURCES = \
diagnostics_util.c  \
ftdi_listener.c  \
ftdi_connect.c  \
ftdi_atomic.c  \
socket_term.c \
socket_com.c  \
util_common.c  \
atomic_queue.c  \
diag_frame_protocol.c  \
fcp_frame_protocol.c  \
kmc_rec_protocol.c  \
ftdi_term.c  \
fcp_term.c  \
msg_handler.c \
ser_ftdi.c \
file_handler.c
