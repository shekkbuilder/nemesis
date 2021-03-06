/*
 * $Id: nemesis-proto_igmp.c,v 1.1.1.1.4.1 2005/01/27 20:14:53 jnathan Exp $
 *
 * THE NEMESIS PROJECT
 * Copyright (C) 1999, 2000, 2001 Mark Grimes <mark@stateful.net>
 * Copyright (C) 2001 - 2003 Jeff Nathan <jeff@snort.org>
 *
 * nemesis-proto_igmp.c (IGMP Packet Generator)
 *
 */

#include "nemesis-igmp.h"
#include "nemesis.h"

int buildigmp(ETHERhdr *eth, IPhdr *ip, IGMPhdr *igmp, struct file *pd,
	      struct file *ipod, libnet_t *l)
{
	static uint8_t *pkt;
	uint32_t        igmp_packetlen = 0;
	uint32_t	igmp_meta_packetlen = 0;
	uint8_t         link_offset = 0;
	int             n;

	if (pd->file_buf == NULL)
		pd->file_len = 0;
	if (ipod->file_buf == NULL)
		ipod->file_len = 0;

	if (got_link)
		link_offset = LIBNET_ETH_H;

	igmp_packetlen      = link_offset + LIBNET_IPV4_H + LIBNET_IGMP_H + pd->file_len + ipod->file_len;
	igmp_meta_packetlen = igmp_packetlen - (link_offset + LIBNET_IPV4_H);

#ifdef DEBUG
	printf("DEBUG: IGMP packet length %u.\n", igmp_packetlen);
	printf("DEBUG: IP   options size  %u.\n", ipod->file_len);
	printf("DEBUG: IGMP payload size  %u.\n", pd->file_len);
#endif

	libnet_build_igmp(igmp->igmp_type,
			  igmp->igmp_code,
			  0,
			  igmp->igmp_group.s_addr,
			  pd->file_buf,
			  pd->file_len,
			  l, 0);

	if (got_ipoptions) {
		if ((libnet_build_ipv4_options(ipod->file_buf, ipod->file_len, l, 0)) == -1)
			fprintf(stderr, "ERROR: Unable to add IP options, discarding them.\n");
	}

	libnet_build_ipv4(igmp_meta_packetlen + LIBNET_IPV4_H,
			  ip->ip_tos,
			  ip->ip_id,
			  ip->ip_off,
			  ip->ip_ttl,
			  ip->ip_p,
			  0,
			  ip->ip_src.s_addr,
			  ip->ip_dst.s_addr,
			  NULL, 0, l, 0);

	if (got_link)
		libnet_build_ethernet(eth->ether_dhost,
				      eth->ether_shost,
				      ETHERTYPE_IP,
				      NULL, 0, l, 0);

	libnet_pblock_coalesce(l, &pkt, &igmp_packetlen);
	n = libnet_write(l);

	if (verbose == 2)
		nemesis_hexdump(pkt, igmp_packetlen, HEX_ASCII_DECODE);
	if (verbose == 3)
		nemesis_hexdump(pkt, igmp_packetlen, HEX_RAW_DECODE);

	if (n != (int)igmp_packetlen) {
		fprintf(stderr, "ERROR: Incomplete packet injection. Only wrote %d bytes.\n", n);
	} else {
		if (verbose) {
			if (got_link)
				printf("Wrote %d byte IGMP packet through linktype %s.\n",
				       n, nemesis_lookup_linktype(l->link_type));
			else
				printf("Wrote %d byte IGMP packet.\n", n);
		}
	}
	libnet_destroy(l);
	return n;
}
