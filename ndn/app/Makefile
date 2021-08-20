APPLICATION ?= app
RIOTBASE ?= $(CURDIR)/../RIOT

BOARD ?= native
DEVELHELP ?= 1

DEFAULT_CHANNEL=15

CFLAGS += -DUSE_LINKLAYER
CFLAGS += -DCCNL_UAPI_H_
CFLAGS += -DUSE_SUITE_NDNTLV
CFLAGS += -DNEEDS_PREFIX_MATCHING
CFLAGS += -DNEEDS_PACKET_CRAFTING

CFLAGS += -DCCNL_CONTENT_TIMEOUT=60
CFLAGS += -DCCNL_FACE_TIMEOUT=3600
CFLAGS += -DCCNL_INTEREST_TIMEOUT=10
CFLAGS += -DCCNL_MAX_INTEREST_RETRANSMIT=3
CFLAGS += -DCCNL_INTEREST_RETRANS_TIMEOUT=2000
CFLAGS += -DCCNL_CACHE_SIZE=48
CFLAGS += -DCCNL_DEFAULT_MAX_PIT_ENTRIES=100
#CFLAGS += -DCCNL_THREAD_PRIORITY="THREAD_PRIORITY_MAIN-3"
CFLAGS += -DCCNL_STACK_SIZE="THREAD_STACKSIZE_DEFAULT+THREAD_EXTRA_STACKSIZE_PRINTF+1536"
CFLAGS += -DCCNL_QUEUE_SIZE=16
CFLAGS += -DHAVE_ASSERT_H
#CFLAGS += -D_NETIF_NETAPI_MSG_QUEUE_SIZE=16

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

USEMODULE += ps
USEMODULE += shell
USEMODULE += shell_commands
# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
# This application dumps received packets to STDIO using the pktdump module
#USEMODULE += gnrc_pktdump
USEMODULE += prng_xorshift
#USEMODULE += netstats_l2
USEMODULE += gnrc_netif_events

USEMODULE += random

USEMODULE += bitfield

USEPKG += ccn-lite

include $(RIOTBASE)/Makefile.include
include $(RIOTMAKE)/default-radio-settings.inc.mk
