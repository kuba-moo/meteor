/*
 * Copyright (c) 2006-2009 The StarBED Project  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/************************************************************************
 *
 * QOMET Emulator Implementation
 *
 * File name: wireconf.c
 * Function: Main source file of the wired-network emulator 
 *           configurator library
 *
 * Authors: Junya Nakata, Razvan Beuran
 * Changes: Kunio AKASHI
 *
 ***********************************************************************/

#include <sys/queue.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <dlfcn.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "wireconf.h"
#include "utils.h"
#include "ip_common.h"
#include "tc_common.h"
#include "tc_util.h"

#define FRAME_LENGTH 1522
#define INGRESS 1 // 1 : ingress mode, 0 : egress mode
#define MAX_DEV 4096
#define DEV_NAME 256
#define OFFSET_RULE 32767

#define TCHK_START(name)           \
struct timeval name##_prev;        \
struct timeval name##_current;     \
gettimeofday(&name##_prev, NULL)

#define TCHK_END(name)                                                             \
    gettimeofday(&name##_current, NULL);                                           \
time_t name##_sec;                                                                 \
suseconds_t name##_usec;                                                           \
if(name##_current.tv_sec == name##_prev.tv_sec) {                                  \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                    \
}                                                                                  \
else if(name ##_current.tv_sec != name##_prev.tv_sec) {                            \
    int name##_carry = 1000000;                                                    \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    if(name##_prev.tv_usec > name##_current.tv_usec) {                             \
        name##_usec = name##_carry - name##_prev.tv_usec + name##_current.tv_usec; \
        name##_sec--;                                                              \
        if(name##_usec > name##_carry) {                                           \
            name##_usec = name##_usec - name##_carry;                              \
            name##_sec++;                                                          \
        }                                                                          \
    }                                                                              \
    else {                                                                         \
        name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                \
    }                                                                              \
}                                                                                  \
printf("%s: sec:%lu usec:%06ld\n", #name, name##_sec, name##_usec);

char ifb_devname[DEV_NAME];
//char ifb_devname[DEV_NAME] = "ifb0";
typedef union {
    uint8_t octet[4];
    uint32_t word;
} in4_addr;

float priv_delay = 0;
double priv_loss = 0;
float priv_rate = 0;

int
get_socket(void)
{
    return rtnl_open(&rth, 0);
}

void
close_socket(socket_id)
int socket_id;
{
    rtnl_close(&rth);
}

// NOT WORKING PROPERLY YET!!!!!!!!!!!!!
int
get_rule(s, rulenum)
uint s;
int16_t rulenum;
{
    int ret;

    ret = system("tc qdisc show");
    if(ret != 0) {
        dprintf(("Cannot print qdisc setting\n"));
    }

    return 0;
}

struct iplink_req {
    struct nlmsghdr     n;
    struct ifinfomsg    i;
    char            buf[1024];
};

int32_t
create_ifb(uint32_t id)
{
    int ret;
    int len;
    int iflatype;
    char *type = "ifb";
    struct iplink_req req;
    struct rtattr *linkinfo;

    snprintf(ifb_devname, DEV_NAME, "ifb%d", id);
    memset(&req, 0, sizeof (req));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof (struct ifinfomsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL;
    req.n.nlmsg_type = RTM_NEWLINK;
    req.i.ifi_family = preferred_family;
    req.i.ifi_index = 0;

    len = strlen(ifb_devname) + 1;
    addattr_l(&req.n, sizeof(req), IFLA_IFNAME, ifb_devname, len);

    linkinfo = NLMSG_TAIL(&req.n);
    addattr_l(&req.n, sizeof (req), IFLA_LINKINFO, NULL, 0);
    addattr_l(&req.n, sizeof (req), IFLA_INFO_KIND, type, strlen(type));

    iflatype = IFLA_INFO_DATA;

    linkinfo->rta_len = (void *)NLMSG_TAIL(&req.n) - (void *)linkinfo;
 
    ret = rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Cannot create ifb device ret: %d\n", ret);
        return 2; 
    }

    return 0;
}

int32_t
delete_ifb()
{
    int ret;
    char cmd[255];

    snprintf(cmd, 255, "ip link del dev %s", ifb_devname);
    ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Cannot delete ifb device\n");
        return 1;
    }

    return 0;
}


int32_t
init_rule(dst, protocol, direction)
char *dst;
int32_t protocol;
int direction;
{
    int32_t i;
    char *devname;
    uint32_t htb_qdisc_id[4];
    uint32_t defcls;

    ll_init_map(&rth);

    set_ifb(ifb_devname, IF_UP);
    change_ifqueuelen(ifb_devname, QLEN);

    for(i = 0; i < if_num; i++) {
        if(!device_list[i].dev_name) {
            break;
        }
        delete_netem_qdisc(device_list[i].dev_name, 1);
        add_ingress_qdisc(device_list[i].dev_name);
        if(add_ingress_filter(device_list[i].dev_name, ifb_devname) != 0) {
            printf("Cannot add ingress filter from %s to %s\n", device_list[i].dev_name, ifb_devname);
        }
    }
    devname = ifb_devname;
    delete_netem_qdisc(devname, 0);

    uint32_t version = 3;
    uint32_t r2q = 8000000000 / 200000;

    if(direction == DIRECTION_BR) {
        defcls = 65535;
    }
    else {
        defcls = 65534;
    }

    htb_qdisc_id[0] = TC_H_ROOT;
    htb_qdisc_id[1] = 0;
    htb_qdisc_id[2] = 1;
    htb_qdisc_id[3] = 0;
    add_htb_qdisc(devname, htb_qdisc_id, version, r2q, defcls);

    uint32_t htb_class_id[4];
    uint32_t netem_qdisc_id[4];
    struct qdisc_params qp;

    // Default Drop rule
    htb_class_id[0] = 1;
    htb_class_id[1] = 0;
    htb_class_id[2] = 1;
    htb_class_id[3] = 65535;
    add_htb_class(devname, htb_class_id, 1000000000);

    netem_qdisc_id[0] = 1;
    netem_qdisc_id[1] = 65535;
    netem_qdisc_id[2] = 65535;
    netem_qdisc_id[3] = 0;
    qp.loss = ~0;
    qp.limit = 100000;
    add_netem_qdisc(devname, netem_qdisc_id, qp);

    // Default Pass rule
    htb_class_id[3] = 65534;
    add_htb_class(devname, htb_class_id, 1000000000);
    netem_qdisc_id[1] = 65534;
    netem_qdisc_id[2] = 65534;
    qp.loss = 0;
    add_netem_qdisc(devname, netem_qdisc_id, qp);

    return 0;
}

int32_t
add_rule_netem(rulenum, handle_nr, protocol, src, dst, direction)
uint16_t rulenum;
int handle_nr;
int32_t protocol;
char *src;
char *dst;
int direction;
{
    char *devname;
    uint32_t htb_class_id[4];
    uint32_t netem_qdisc_id[4];
    uint32_t filter_id[4];
    struct qdisc_params qp;
    struct u32_params ufp;

    if(!INGRESS) {
        devname = (char* )malloc(DEV_NAME);
        devname =  (char* )get_route_info("dev", dst);
    }
    else if(INGRESS) {
        devname = ifb_devname;
    }

    memset(&qp, 0, sizeof(struct qdisc_params));
    memset(&ufp, 0, sizeof(struct u32_params));

    dprintf(("[add_rule] rulenum = %d\n", handle_nr));
    qp.limit = 100000;
    qp.delay = 0.001;
    qp.rate = Gigabit;
    qp.buffer = Gigabit / 1000;

    char srcaddr[20];
    char dstaddr[20];
    char bcastaddr[20];

    htb_class_id[0] = 1;
    htb_class_id[1] = 0;
    htb_class_id[2] = 1;
    htb_class_id[3] = handle_nr;

    netem_qdisc_id[0] = 1;
    netem_qdisc_id[1] = handle_nr;
    netem_qdisc_id[2] = handle_nr;
    netem_qdisc_id[3] = 0;

    add_htb_class(devname, htb_class_id, 1000000000);
    add_netem_qdisc(devname, netem_qdisc_id, qp);

    filter_id[0] = 1;
    filter_id[1] = 0;
    filter_id[2] = 1;
    filter_id[3] = handle_nr;

    ufp.classid[0] = filter_id[2];
    ufp.classid[1] = filter_id[3];
    ufp.match[IP_SRC].filter = "src";
    ufp.match[IP_DST].filter = "dst";

    if(protocol == ETH) {
        sprintf(bcastaddr, "%s", "ff:ff:ff:ff:ff:ff");
    }
    else if(protocol == IP) {
        sprintf(bcastaddr, "%s", "255.255.255.255/32");
    }

    if(src != NULL) {
        if(protocol == ETH) {
            sprintf(srcaddr, "%s", src);
            ufp.match[IP_SRC].proto = "ether";
        }
        else if(protocol == IP) {
            sprintf(srcaddr, "%s", src);
            ufp.match[IP_SRC].proto = "ip";
        }
        dprintf(("[add_rule] filter source address : %s\n", srcaddr));
    }
    else {
        dprintf(("[add_rule] source address is NULL\n"));
    }
    if(dst != NULL) {
        if(protocol == ETH) {
            sprintf(dstaddr, "%s", dst);
            ufp.match[IP_DST].proto = "ether";
            ufp.match[IP_DST].offmask = 0;
        }
        else if(protocol == IP) {
            sprintf(dstaddr, "%s", dst);
            ufp.match[IP_DST].proto = "ip";
            ufp.match[IP_DST].offmask = 0;
        }
        dprintf(("[add_rule] filter dstination address : %s\n", dstaddr));
    }
    else {
        dprintf(("[add_rule] destination address is NULL\n"));
    }

    if(strcmp(src, "any") == 0) {
        strcpy(srcaddr, "0.0.0.0/0");
    }
    if(strcmp(dst, "any") == 0) {
        strcpy(dstaddr, "0.0.0.0/0");
    }

    if(direction != DIRECTION_IN) {
        if(strcmp(src, "any") == 0) {
            ufp.match[IP_SRC].type = NULL;
        }
        else {
            ufp.match[IP_SRC].type = "u32";
        }
        if(strcmp(dst, "any") == 0) {
            ufp.match[IP_DST].type = NULL;
        }
        else {
            ufp.match[IP_DST].type = "u32";
        }
        ufp.match[IP_SRC].arg = srcaddr;
        ufp.match[IP_DST].arg = dstaddr;
        add_tc_filter(devname, filter_id, "all", "u32", &ufp);
    }

    if(strcmp(dst, "any") == 0) {
        ufp.match[IP_SRC].type = NULL;
    }
    else {
        ufp.match[IP_SRC].type = "u32";
    }
    if(strcmp(src, "any") == 0) {
        ufp.match[IP_DST].type = NULL;
    }
    else {
        ufp.match[IP_DST].type = "u32";
    }
    ufp.match[IP_SRC].arg = dstaddr;
    ufp.match[IP_DST].arg = srcaddr;
    add_tc_filter(devname, filter_id, "all", "u32", &ufp);

    if(direction != DIRECTION_IN) {
        ufp.match[IP_SRC].arg = srcaddr;
        ufp.match[IP_DST].arg = bcastaddr;

        add_tc_filter(devname, filter_id, "all", "u32", &ufp);
    }

    return 0;
}

int32_t 
add_rule(s, rulenum, pipe_nr, protocol, src, dst, direction)
int s;
uint32_t rulenum;
int pipe_nr;
int32_t protocol;
char *src;
char *dst;
int direction;
{
    return add_rule_netem(rulenum, pipe_nr, protocol, src, dst, direction);
}

int
delete_netem(s, dst, rule_number)
uint32_t s;
char* dst;
uint32_t rule_number;
{
    char* devname;
    int32_t i;
    struct qdisc_params qp; 

    memset(&qp, 0, sizeof(qp));
    devname = malloc(DEV_NAME);

    for(i = 0; i < if_num; i++) {
        if(!device_list[i].dev_name) {
            break;
        }
        delete_netem_qdisc(device_list[i].dev_name, INGRESS);
    }
    delete_netem_qdisc(ifb_devname, 0);

    return SUCCESS;
}

int
delete_rule(s, dst, rule_number)
uint32_t s;
char* dst;
uint32_t rule_number;
{
    return delete_netem(s, dst, rule_number);
}

int
configure_qdisc(handle, bandwidth, delay, lossrate)
int32_t handle;
int32_t bandwidth;
float delay;
double lossrate;
{
    char *devname;
    int32_t ret;
    uint32_t htb_class_id[4];
    uint32_t netem_qdisc_id[4];
    struct qdisc_params qp;

    memset(&qp, 0, sizeof(qp));
    devname = malloc(DEV_NAME);

    htb_class_id[0] = 1;
    htb_class_id[1] = 0;
    htb_class_id[2] = 1;
    htb_class_id[3] = handle;

    netem_qdisc_id[0] = 1;
    netem_qdisc_id[1] = handle;
    netem_qdisc_id[2] = handle;
    netem_qdisc_id[3] = 0;

    devname = ifb_devname;

    qp.delay = delay;
    qp.limit = 14880000; // 14Mpps = 10Gbps(64byte/packet)
    if(lossrate == 1) {
        qp.loss = ~0;
    }
    else if(lossrate < 1 || lossrate > 0) {
        qp.loss = lossrate * max_percent_value;
    }
    else {
        qp.loss = 0;
    }
    ret = change_netem_qdisc(devname, netem_qdisc_id, qp);
    if(ret != 0) {
        fprintf(stderr, "Cannot change netem disc\n");
        return ret;
    }

    qp.rate = bandwidth;
    if((bandwidth / 1024) < FRAME_LENGTH) {
        qp.buffer = FRAME_LENGTH;
    }
    else {
        qp.buffer = FRAME_LENGTH / 1024;
    }
    ret = change_htb_class(devname, htb_class_id, qp.rate);
    if(ret != 0) {
        fprintf(stderr, "Cannot change HTB class\n");
        return ret;
    }

    return SUCCESS;
}

int32_t
configure_rule(dsock, pipe_nr, bandwidth, delay, lossrate)
int dsock;
int32_t pipe_nr;
int32_t bandwidth;
double delay;
double lossrate;
{
    return configure_qdisc(pipe_nr, bandwidth, delay, lossrate);
}
