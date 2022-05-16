AM_CFLAGS = $(EXTRA_CFLAGS) -I$(top_srcdir)/CDM_v2.12.36.4_WHQL_Certified
LDADD = $(EXTRA_LDFLAGS) -L$(top_srcdir)/CDM_v2.12.36.4_WHQL_Certified/amd64/ -lftd2xx$(HOST_OS_FLAG)
LIB = 


bin_PROGRAMS = ser_ftdi

ser_ftdi_SOURCES = diagnostics.c ftdi_connect.c ftdi_atomic.c fcp_frame_protocol.c kmc_rec_protocol.c ftdi_term.c ftdi_listener.c ser_ftdi.c
