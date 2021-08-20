#include <stdio.h>

#include "msg.h"
#include "shell.h"
#include "ccn-lite-riot.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/pktdump.h"
#include "ccnl-callbacks.h"
#include "ccnl-producer.h"
#include "random.h"
#include "bitfield.h"
#include "mutex.h"

/* main thread's message queue */
#define MAIN_QUEUE_SIZE     (32)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

typedef struct {
    signed version;
    uint16_t max_chunks;
    uint16_t chunk_size;
} manifest_t;

manifest_t mymanifest;

gnrc_netif_t *mynetif;
uint8_t hwaddr[GNRC_NETIF_L2ADDR_MAXLEN];
char hwaddr_str[GNRC_NETIF_L2ADDR_MAXLEN * 3];

static uint16_t myclass;

static unsigned char int_buf[CCNL_MAX_PACKET_SIZE];
static unsigned char data_buf[CCNL_MAX_PACKET_SIZE];

static uint16_t max_chunks = 1000;
static uint16_t chunk_size = 32;

extern void install_routes(char *laddr, char *toaddr, char *nhaddr);

static uint16_t _my_id;

static uint8_t chunk_memory[32];
static uint8_t manifest_memory[32];

static BITFIELD(chunk_buffer, 4000U);
static mutex_t cb_mutex;

bool cascade = false;

static const char *gwaddrs[] = GWADDRS;
static bool gw_node = false;

int myversion = -1;
unsigned _my_pid;

extern struct ccnl_face_s *loopback_face;

uint16_t get_id(char *address)
{
#define MYMAP(ID,ADDR)                                           \
    if (!memcmp(ADDR, address, strlen(ADDR))) {                  \
        return ID;                                               \
    }
#include "idaddr.inc"
#undef MYMAP
    return 0;
}

void on_interest_retransmit(struct ccnl_relay_s *relay, struct ccnl_interest_s *ccnl_int)
{
    (void) relay;
    uint32_t now;
    if (!memcmp(ccnl_int->pkt->pfx->comp[4], "chunk", ccnl_int->pkt->pfx->complen[4])) {

        char buf[8] = { 0 };

        memset(buf, 0, sizeof(buf));
        memcpy(buf, ccnl_int->pkt->pfx->comp[3], ccnl_int->pkt->pfx->complen[3]);
        int ver = atoi(buf);

        memset(buf, 0, sizeof(buf));
        memcpy(buf, ccnl_int->pkt->pfx->comp[5], ccnl_int->pkt->pfx->complen[5]);
        int chunkid = atoi(buf);

        now = (xtimer_now_usec64() / US_PER_MS) & UINT32_MAX;

        printf("ctxqq;%lu;%03d;%04u;%04u\n", now, (int) ver, (unsigned) chunkid, (unsigned) max_chunks);
    }
}

static struct ccnl_content_s *generate_chunk(struct ccnl_relay_s *relay, struct ccnl_pkt_s *pkt)
{
    (void) relay;
    size_t offs = CCNL_MAX_PACKET_SIZE;

    size_t reslen = 0;

    ccnl_ndntlv_prependContent(pkt->pfx, (unsigned char*) chunk_memory, chunk_size, NULL, NULL, &offs, data_buf, &reslen);

    size_t len = sizeof(data_buf);

    unsigned char *olddata;
    unsigned char *data = olddata = data_buf + offs;

    uint64_t typ;

    if (ccnl_ndntlv_dehead(&data, &reslen, &typ, &len) || typ != NDN_TLV_Data) {
        puts("ERROR in producer_func");
        return 0;
    }

    struct ccnl_content_s *c = 0;
    struct ccnl_pkt_s *pk = ccnl_ndntlv_bytes2pkt(typ, olddata, &data, &reslen);
    c = ccnl_content_new(&pk);

    return c;
}

int cache_decision(struct ccnl_relay_s *relay, struct ccnl_content_s *c)
{
    (void) relay;
    /* /deployment/vendor/class/<version>/chunk/<id> */
    if (!gw_node && !memcmp(c->pkt->pfx->comp[4], "chunk", c->pkt->pfx->complen[4])) {
        char buf[8] = { 0 };
        memcpy(buf, c->pkt->pfx->comp[2], c->pkt->pfx->complen[2]);
        unsigned class = atoi(buf);
        memset(buf, 0, sizeof(buf));
        memcpy(buf, c->pkt->pfx->comp[3], c->pkt->pfx->complen[3]);
        int ver = atoi(buf);
        memset(buf, 0, sizeof(buf));
        memcpy(buf, c->pkt->pfx->comp[5], c->pkt->pfx->complen[5]);
        unsigned chunkid = atoi(buf);
        if ((ver == myversion) && (class == myclass)) {
            bool isset = false;
            mutex_lock(&cb_mutex);
            isset = bf_isset(chunk_buffer, chunkid);
            mutex_unlock(&cb_mutex);

            if (isset) {
                return 0;
            }
        }
    }

    return 1;
}

int on_data(struct ccnl_relay_s *relay,
            struct ccnl_face_s *from,
            struct ccnl_pkt_s *pkt) {
    (void) relay;
    (void) from;
    uint32_t now;

    char buf[8] = { 0 };
    memcpy(buf, pkt->pfx->comp[3], pkt->pfx->complen[3]);
    int ver = atoi(buf);

    now = (xtimer_now_usec64() / US_PER_MS) & UINT32_MAX;

    if (gw_node) {
        printf("mrxp;%lu;%03d;%03d\n", now, (int) myversion, (int) ver);
        return 0;
    }

    if (!memcmp(pkt->pfx->comp[4], "manifest", pkt->pfx->complen[4])) {
        if (ver > myversion) {
            mymanifest.version = ver;
            mymanifest.max_chunks = *((uint16_t *)(pkt->content));

            printf("mrxp;%lu;%03d;%03d\n", now, (int) myversion, (int) ver);

            msg_t m = { .type = 3, .content.ptr = &mymanifest };
            msg_send(&m, _my_pid);
        }
    }

    /* /deployment/vendor/class/<version>/chunk/<id> */
    if (!gw_node && !memcmp(pkt->pfx->comp[4], "chunk", pkt->pfx->complen[4])) {
        char buf[8] = { 0 };
        memcpy(buf, pkt->pfx->comp[2], pkt->pfx->complen[2]);
        unsigned class = atoi(buf);
        memset(buf, 0, sizeof(buf));
        memcpy(buf, pkt->pfx->comp[3], pkt->pfx->complen[3]);
        int ver = atoi(buf);
        memset(buf, 0, sizeof(buf));
        memcpy(buf, pkt->pfx->comp[5], pkt->pfx->complen[5]);
        unsigned chunkid = atoi(buf);

        if ((ver == myversion) && (class == myclass)) {
            mutex_lock(&cb_mutex);
            bf_set(chunk_buffer, chunkid);
            mutex_unlock(&cb_mutex);

            now = (xtimer_now_usec64() / US_PER_MS) & UINT32_MAX;
            printf("crxp;%lu;%03d;%04u\n", now, (int) ver, (unsigned) chunkid);

            msg_t m = { .type = 4 };
            msg_send(&m, _my_pid);
        }
    }

    return 0;
}

int producer(struct ccnl_relay_s *relay,
             struct ccnl_face_s *from,
             struct ccnl_pkt_s *pkt) {
    uint32_t now;

    /* /deployment/vendor/class/<version>/manifest */
    if (!memcmp(pkt->pfx->comp[4], "manifest", pkt->pfx->complen[4])) {
        if (!gw_node) {

            char buf[4] = { 0 };
            memcpy(buf, pkt->pfx->comp[3], pkt->pfx->complen[3]);
            int ver = atoi(buf);

            now = (xtimer_now_usec64() / US_PER_MS) & UINT32_MAX;

            if (from == loopback_face) {
                now = (xtimer_now_usec64() / US_PER_MS) & UINT32_MAX;
                printf("mtxq;%lu;%03d\n", now, (int)ver);
                return 0;
            }

            printf("mrxq;%lu;%03d;%03d\n", now, (int)myversion, (int)ver);

            if (ver > myversion+1){
                msg_t m = { .type = 1, .content.value = ver };
                msg_send(&m, _my_pid);
            }
            return 0;
        }
    }

    /* /deployment/vendor/class/<version>/chunk/<id> */
    if (!memcmp(pkt->pfx->comp[4], "chunk", pkt->pfx->complen[4])) {

        char buf[8] = { 0 };

        memcpy(buf, pkt->pfx->comp[2], pkt->pfx->complen[2]);
        unsigned class = atoi(buf);

        memset(buf, 0, sizeof(buf));
        memcpy(buf, pkt->pfx->comp[3], pkt->pfx->complen[3]);
        int ver = atoi(buf);

        memset(buf, 0, sizeof(buf));
        memcpy(buf, pkt->pfx->comp[5], pkt->pfx->complen[5]);
        int chunkid = atoi(buf);

        now = (xtimer_now_usec64() / US_PER_MS) & UINT32_MAX;

        if (from == loopback_face) {
            now = (xtimer_now_usec64() / US_PER_MS) & UINT32_MAX;
            printf("ctxq;%lu;%03d;%04u;%04u\n", now, (int) myversion, (unsigned) chunkid, (unsigned) max_chunks);
            return 0;
        }

        printf("crxq;%lu;%03d;%04u\n", now, ver, (unsigned) chunkid);

        if (!gw_node) {
            if (ver > myversion) {
                msg_t m = { .type = 1, .content.value = ver };
                msg_send(&m, _my_pid);
                return 0;
            }
            else if (ver < myversion) {
                ccnl_pkt_free(pkt);
                return 1;
            }
        }

        bool issetmax = false, isset = false;

        mutex_lock(&cb_mutex);
        issetmax = bf_isset(chunk_buffer, max_chunks-1);
        isset = bf_isset(chunk_buffer, chunkid);
        mutex_unlock(&cb_mutex);

        if (cascade && !issetmax) {
            /* ignore chunk requests as long as we are retrieving them */
            return 1;
        }

        if (isset && (gw_node || (class == myclass))) {
            struct ccnl_content_s *c = generate_chunk(relay, pkt);
            if (c) {
                uint32_t now = (xtimer_now_usec64() / US_PER_MS) & UINT32_MAX;
                printf("ctxp;%lu;%03d;%04u\n", now, (int) myversion, (unsigned) chunkid);
                ccnl_send_pkt(relay, from, c->pkt);
                ccnl_content_free(c);
                return 1;
            }
        }
    }

    return 0;
}

static void send_static_request(int version)
{
    char req_uri[48];
    struct ccnl_prefix_s *prefix = NULL;

    memset(int_buf, 0, CCNL_MAX_PACKET_SIZE);
    snprintf(req_uri, sizeof(req_uri), "/Deployment/Vendor/Class/%03d/manifest", (int)version);
    prefix = ccnl_URItoPrefix(req_uri, CCNL_SUITE_NDNTLV, NULL);

    ccnl_send_interest(prefix, int_buf, CCNL_MAX_PACKET_SIZE, NULL);
    ccnl_prefix_free(prefix);
}

static void request_chunk(int version, unsigned chunk)
{
    char req_uri[48];
    struct ccnl_prefix_s *prefix = NULL;

    memset(int_buf, 0, CCNL_MAX_PACKET_SIZE);
    snprintf(req_uri, sizeof(req_uri), "/Deployment/Vendor/%05d/%03d/chunk/%04u", (unsigned) myclass, (int) version, (unsigned) chunk);
    prefix = ccnl_URItoPrefix(req_uri, CCNL_SUITE_NDNTLV, NULL);

    ccnl_send_interest(prefix, int_buf, CCNL_MAX_PACKET_SIZE, NULL);
    ccnl_prefix_free(prefix);
}

void _generate_version(int version)
{
    char req_uri[48];
    struct ccnl_prefix_s *prefix = NULL;

    memset(manifest_memory, 0, sizeof(manifest_memory)/sizeof(manifest_memory[0]));
    uint16_t *mptr = (uint16_t *) manifest_memory;
    *mptr = max_chunks;
    memset(int_buf, 0, CCNL_MAX_PACKET_SIZE);
    snprintf(req_uri, sizeof(req_uri), "/Deployment/Vendor/Class/%03d/manifest", (int)version);
    prefix = ccnl_URItoPrefix(req_uri, CCNL_SUITE_NDNTLV, NULL);

    size_t offs = CCNL_MAX_PACKET_SIZE;
    size_t reslen = 0;
    ccnl_ndntlv_prependContent(prefix, (unsigned char*) manifest_memory, sizeof(manifest_memory), NULL, NULL, &offs, int_buf, &reslen);

    ccnl_prefix_free(prefix);

    unsigned char *olddata;
    unsigned char *data = olddata = int_buf + offs;

    size_t len;
    uint64_t typ;

    if (ccnl_ndntlv_dehead(&data, &reslen, &typ, &len) ||
        typ != NDN_TLV_Data) {
        return;
    }

    struct ccnl_content_s *c = 0;
    struct ccnl_pkt_s *pk = ccnl_ndntlv_bytes2pkt(typ, olddata, &data, &reslen);
    c = ccnl_content_new(&pk);
    c->flags |= CCNL_CONTENT_FLAGS_STATIC;

    msg_t m = { .type = CCNL_MSG_CS_ADD, .content.ptr = c };

    if(msg_send(&m, ccnl_event_loop_pid) < 1){
        puts("could not add content");
    }
}

static int _handle_versions(void)
{
    evtimer_t evtimer;
    evtimer_msg_event_t event_chunk, event_manifest_req;
    unsigned chunk = 0;

    evtimer_init_msg(&evtimer);

    event_chunk.msg.type = 4;

    if (!gw_node) {
        event_manifest_req.event.offset = random_uint32_range(5000, 15000);
        event_manifest_req.msg.type = 1;
        evtimer_add_msg(&evtimer, &event_manifest_req, thread_getpid());
    }
    else {
        myversion++;
        uint32_t now = (xtimer_now_usec64() / US_PER_MS) & UINT32_MAX;
        printf("mgen;%lu;%03d;%04d;%04d\n", now, (int) myversion, (unsigned) max_chunks, (unsigned) chunk_size);
        _generate_version(myversion);
    }

    manifest_t *mani = NULL;

    while (1) {
        msg_t m;
        msg_receive(&m);
        switch (m.type) {
        case 1:
            send_static_request(myversion+1);
            if (myversion < 0) {
                event_manifest_req.event.offset = random_uint32_range(5000, 15000);
                event_manifest_req.msg.type = 1;
                evtimer_del(&evtimer, &(event_manifest_req.event));
                evtimer_add_msg(&evtimer, &event_manifest_req, thread_getpid());
            }

            break;
        case 3:
            /* received new version manifest */

            mani = (manifest_t *) m.content.ptr;
            max_chunks = mani->max_chunks;

            myversion = mani->version;

            /* delete old (on-going) chunk progress */
            mutex_lock(&cb_mutex);
            memset(chunk_buffer, 0x00, sizeof(chunk_buffer));
            mutex_unlock(&cb_mutex);
            evtimer_del(&evtimer, &(event_chunk.event));

            m.type = 4;
            msg_send(&m, thread_getpid());

            break;
        case 4:
            /* retrieve chunk */
            evtimer_del(&evtimer, &(event_chunk.event));
            mutex_lock(&cb_mutex);
            for (chunk = 0; chunk < max_chunks; chunk++) {
                if (!bf_isset(chunk_buffer, chunk)) {
                    break;
                }
            }
            mutex_unlock(&cb_mutex);

            if (chunk < max_chunks) {
                request_chunk(myversion, chunk);
                event_chunk.event.offset = random_uint32_range(10000, 15000);
                evtimer_add_msg(&evtimer, &event_chunk, thread_getpid());
            }

            break;
        }
    }
    return 0;
}

static int _start_exp(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    puts("start");

    max_chunks = atoi(argv[1]);
    cascade = (bool) atoi(argv[2]);

    _handle_versions();
    printf("end\n");

    return 0;
}

static const shell_command_t shell_commands[] = {
    { "startexp", "", _start_exp },
    { NULL, NULL, NULL }
};

int main(void)
{
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    ccnl_core_init();

    ccnl_start();

    mynetif = gnrc_netif_iter(NULL);
    ccnl_open_netif(mynetif->pid, GNRC_NETTYPE_CCN);

    uint16_t src_len = 8U;
    gnrc_netapi_set(mynetif->pid, NETOPT_SRC_LEN, 0, &src_len, sizeof(src_len));
#ifdef BOARD_NATIVE
    gnrc_netapi_get(mynetif->pid, NETOPT_ADDRESS, 0, hwaddr, sizeof(hwaddr));
#else
    gnrc_netapi_get(mynetif->pid, NETOPT_ADDRESS_LONG, 0, hwaddr, sizeof(hwaddr));
#endif
    gnrc_netif_addr_to_str(hwaddr, sizeof(hwaddr), hwaddr_str);


    for (unsigned i = 0; i < sizeof(gwaddrs) / sizeof(gwaddrs[0]); i++) {
        if (!strcmp(gwaddrs[i], hwaddr_str)) {
            gw_node=true;
        }
    }

    _my_id = get_id(hwaddr_str);
    _my_pid = thread_getpid();

    mutex_lock(&cb_mutex);
    if (gw_node) {
        memset(chunk_buffer, 0xFF, sizeof(chunk_buffer));
    }
    else {
        memset(chunk_buffer, 0x00, sizeof(chunk_buffer));
    }

    mutex_unlock(&cb_mutex);

    memset(chunk_memory, 0x00, sizeof(chunk_memory));

    uint32_t now = (xtimer_now_usec64() / US_PER_MS) & UINT32_MAX;
    printf("addr;%lu;%u;%s;%s;%s;%u\n", now, _my_id, hwaddr_str, "", "", gw_node);

    ccnl_set_local_producer(producer);
    ccnl_set_cb_rx_on_data(on_data);
    ccnl_set_cache_strategy_cache(cache_decision);

#define ROUTE(myid, laddr, toaddr, nhaddr) install_routes(laddr, toaddr, nhaddr);
#include "routesdown.inc"
#undef ROUTE

    random_init(*((uint32_t *)hwaddr));

//    myclass = random_uint32() % UINT16_MAX;
    myclass = 0;

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}
