/*
 * Copyright (C) 2014 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: Michal Privoznik <mprivozn@redhat.com>
 */

#include <config.h>

#include "testutils.h"
#include "vircommand.h"
#include "virnetdevbandwidth.h"
#include "netdev_bandwidth_conf.c"

#define VIR_FROM_THIS VIR_FROM_NONE

struct testMinimalStruct {
    const char *expected_result;
    const char *band1;
    const char *band2;
};

struct testSetStruct {
    const char *band;
    const char *exp_cmd;
    const char *iface;
    const bool hierarchical_class;
};

#define PARSE(xml, var)                                                 \
    do {                                                                \
        xmlDocPtr doc;                                                  \
        xmlXPathContextPtr ctxt = NULL;                                 \
                                                                        \
        if (!xml)                                                       \
            break;                                                      \
                                                                        \
        if (!(doc = virXMLParseStringCtxt((xml),                        \
                                          "bandwidth definition",       \
                                          &ctxt)))                      \
            goto cleanup;                                               \
                                                                        \
        (var) = virNetDevBandwidthParse(ctxt->node,                     \
                                        VIR_DOMAIN_NET_TYPE_NETWORK);   \
        xmlFreeDoc(doc);                                                \
        xmlXPathFreeContext(ctxt);                                      \
        if (!(var))                                                     \
            goto cleanup;                                               \
    } while (0)

static int
testVirNetDevBandwidthMinimal(const void *data)
{
    int ret = -1;
    const struct testMinimalStruct *info = data;
    virNetDevBandwidthPtr expected_result = NULL, result = NULL,
                          band1 = NULL, band2 = NULL;


    /* Parse given XMLs */
    PARSE(info->expected_result, expected_result);
    PARSE(info->band1, band1);
    PARSE(info->band2, band2);

    if (virNetDevBandwidthMinimal(&result, band1, band2) < 0)
        goto cleanup;

    if (!virNetDevBandwidthEqual(expected_result, result)) {
        virBuffer exp_buf = VIR_BUFFER_INITIALIZER,
                  res_buf = VIR_BUFFER_INITIALIZER;
        char *exp = NULL, *res = NULL;

        fprintf(stderr, "expected_result != result");

        if (virNetDevBandwidthFormat(expected_result, &exp_buf) < 0 ||
            virNetDevBandwidthFormat(result, &res_buf) < 0 ||
            !(exp = virBufferContentAndReset(&exp_buf)) ||
            !(res = virBufferContentAndReset(&res_buf))) {
            fprintf(stderr, "Failed to fail");
            virBufferFreeAndReset(&exp_buf);
            virBufferFreeAndReset(&res_buf);
            VIR_FREE(exp);
            VIR_FREE(res);
            goto cleanup;
        }

        virtTestDifference(stderr, exp, res);
        VIR_FREE(exp);
        VIR_FREE(res);
    }

    ret = 0;
cleanup:
    virNetDevBandwidthFree(expected_result);
    virNetDevBandwidthFree(result);
    virNetDevBandwidthFree(band1);
    virNetDevBandwidthFree(band2);
    return ret;
}

static int
testVirNetDevBandwidthSet(const void *data)
{
    int ret = -1;
    const struct testSetStruct *info = data;
    const char *iface = info->iface;
    virNetDevBandwidthPtr band = NULL;
    virBuffer buf = VIR_BUFFER_INITIALIZER;
    char *actual_cmd = NULL;

    PARSE(info->band, band);

    if (!iface)
        iface = "eth0";

    virCommandSetDryRun(&buf);

    if (virNetDevBandwidthSet(iface, band, info->hierarchical_class) < 0)
        goto cleanup;

    if (!(actual_cmd = virBufferContentAndReset(&buf))) {
        int err = virBufferError(&buf);
        if (err) {
            fprintf(stderr, "buffer's in error state: %d", err);
            goto cleanup;
        }
        /* This is interesting, no command has been executed.
         * Maybe that's expected, actually. */
    }

    if (STRNEQ_NULLABLE(info->exp_cmd, actual_cmd)) {
        virtTestDifference(stderr, info->exp_cmd, actual_cmd);
        goto cleanup;
    }

    ret = 0;
cleanup:
    virNetDevBandwidthFree(band);
    virBufferFreeAndReset(&buf);
    VIR_FREE(actual_cmd);
    return ret;
}

static int
mymain(void)
{
    int ret = 0;

#define DO_TEST_MINIMAL(r, ...)                             \
    do {                                                    \
        struct testMinimalStruct data = {r, __VA_ARGS__};   \
        if (virtTestRun("virNetDevBandwidthMinimal",        \
                        testVirNetDevBandwidthMinimal,      \
                        &data) < 0)                         \
            ret = -1;                                       \
    } while (0)

#define DO_TEST_SET(Band, Exp_cmd, ...)                     \
    do {                                                    \
        struct testSetStruct data = {.band = Band,          \
                                     .exp_cmd = Exp_cmd,    \
                                     __VA_ARGS__};          \
        if (virtTestRun("virNetDevBandwidthSet",            \
                        testVirNetDevBandwidthSet,          \
                        &data) < 0)                         \
            ret = -1;                                       \
    } while (0)


    DO_TEST_MINIMAL(NULL, NULL, NULL);

    DO_TEST_MINIMAL("<bandwidth>"
                    "  <inbound average='1000' peak='5000' burst='5120'/>"
                    "  <outbound average='128' peak='256' burst='256'/>"
                    "</bandwidth>",
                    .band1 = "<bandwidth>"
                    "  <inbound average='1000' peak='5000' burst='5120'/>"
                    "  <outbound average='128' peak='256' burst='256'/>"
                    "</bandwidth>");

    DO_TEST_MINIMAL("<bandwidth>"
                    "  <inbound average='1000' peak='5000' burst='5120'/>"
                    "  <outbound average='128' peak='256' burst='256'/>"
                    "</bandwidth>",
                    .band2 = "<bandwidth>"
                    "  <inbound average='1000' peak='5000' burst='5120'/>"
                    "  <outbound average='128' peak='256' burst='256'/>"
                    "</bandwidth>");
    DO_TEST_MINIMAL("<bandwidth>"
                    "  <inbound average='1' peak='2' floor='3' burst='4'/>"
                    "  <outbound average='5' peak='6' burst='7'/>"
                    "</bandwidth>",
                    "<bandwidth>"
                    "  <inbound average='1' peak='2' burst='4'/>"
                    "  <outbound average='0' burst='7'/>"
                    "</bandwidth>",
                    "<bandwidth>"
                    "  <inbound average='1' peak='2' floor='3'/>"
                    "  <outbound average='5' peak='6'/>"
                    "</bandwidth>");

    DO_TEST_SET(("<bandwidth>"
                 "  <inbound average='1' peak='2' floor='3' burst='4'/>"
                 "  <outbound average='5' peak='6' burst='7'/>"
                 "</bandwidth>"),
                ("/sbin/tc qdisc del dev eth0 root\n"
                 "/sbin/tc qdisc del dev eth0 ingress\n"
                 "/sbin/tc qdisc add dev eth0 root handle 1: htb default 1\n"
                 "/sbin/tc class add dev eth0 parent 1: classid 1:1 htb rate 1kbps ceil 2kbps burst 4kb\n"
                 "/sbin/tc qdisc add dev eth0 parent 1:1 handle 2: sfq perturb 10\n"
                 "/sbin/tc filter add dev eth0 parent 1:0 protocol ip handle 1 fw flowid 1\n"
                 "/sbin/tc qdisc add dev eth0 ingress\n"
                 "/sbin/tc filter add dev eth0 parent ffff: protocol ip u32 match ip src 0.0.0.0/0 "
                 "police rate 5kbps burst 7kb mtu 64kb drop flowid :1\n"));

    return ret;
}

VIRT_TEST_MAIN(mymain);
