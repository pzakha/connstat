/*
 * Connstat - A kernel module to gather TCP4 connection statistics and
 * make them available in /proc/net/stats_tcp.
 *
 * This code relies on linux/net/ipv4/tcp_ipv4.c.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <net/tcp.h>
#include <linux/proc_fs.h>

typedef char ipv4_ip[15];

#define GET_OCTET0(hip) (hip & 0xFF)
#define GET_OCTET1(hip) ((hip>>8) & 0xFF)
#define GET_OCTET2(hip) ((hip>>16) & 0xFF)
#define GET_OCTET3(hip) (hip>>24)

static char *tcp_state_strings[] = {
    "NONE",
    "ESTABLISHED",
    "SYN_SENT",
    "SYN_RECEIVED",
    "FIN_WAIT1",
    "FIN_WAIT2",
    "TIME_WAIT",
    "CLOSED",
    "CLOSE_WAIT",
    "LAST_ACK",
    "LISTEN",
    "CLOSING",
    "NEW_SYN_RECV",
    "MAX_STATES"
};

/* Convert hex ip to ipv4(XXX.XXX.XXX.XXX) */
void hex_to_ipv4_ip(__be32 hip, ipv4_ip v4ip)
{
	sprintf(v4ip,"%d.%d.%d.%d", GET_OCTET0(hip), GET_OCTET1(hip),
	    GET_OCTET2(hip), GET_OCTET3(hip));
}

static void record_ipv4_route(struct seq_file *f, __be32 src,
    __u16 srcp, __be32 dest, __u16 destp, int state, u32 cwnd, u32 rwnd,
    u32 swnd, u32 mss, u64 inbytes, u32 insegs, u64 outbytes, u32 outsegs,
    u32 retranssegs, u32 rto, u32 rtt, u32 suna, u32 unsent)
{
	ipv4_ip laddr, raddr;

	hex_to_ipv4_ip(src, laddr);
	hex_to_ipv4_ip(dest, raddr);

        seq_printf(f, "%s,%u,%s,%u,%s,%llu,%u,%llu,%u,%u,"
	    "%u,%u,%u,%u,%u,%u,%u,%u",
	    laddr, srcp, raddr, destp, tcp_state_strings[state],
	    inbytes, insegs, outbytes, outsegs, retranssegs,
	    suna, unsent, swnd, cwnd, rwnd, mss, rto, rtt);
}


static void get_openreq4(const struct request_sock *req,
                         struct seq_file *f)
{
        const struct inet_request_sock *ireq = inet_rsk(req);

	record_ipv4_route(f, ireq->ir_loc_addr, ireq->ir_num,
	    ireq->ir_rmt_addr, ntohs(ireq->ir_rmt_port), TCP_SYN_RECV, 0, 0,
	    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

}

static void get_tcp4_sock(struct sock *sk, struct seq_file *f)
{
        int timer_active;
        unsigned long timer_expires;
        const struct tcp_sock *tp = tcp_sk(sk);
        const struct inet_connection_sock *icsk = inet_csk(sk);
        const struct inet_sock *inet = inet_sk(sk);
        __be32 dest = inet->inet_daddr;
        __be32 src = inet->inet_rcv_saddr;
        __u16 destp = ntohs(inet->inet_dport);
        __u16 srcp = ntohs(inet->inet_sport);
        int rx_queue;
        int state;

        if (icsk->icsk_pending == ICSK_TIME_RETRANS ||
            icsk->icsk_pending == ICSK_TIME_REO_TIMEOUT ||
            icsk->icsk_pending == ICSK_TIME_LOSS_PROBE) {
                timer_active    = 1;
                timer_expires   = icsk->icsk_timeout;
        } else if (icsk->icsk_pending == ICSK_TIME_PROBE0) {
                timer_active    = 4;
                timer_expires   = icsk->icsk_timeout;
        } else if (timer_pending(&sk->sk_timer)) {
                timer_active    = 2;
                timer_expires   = sk->sk_timer.expires;
        } else {
                timer_active    = 0;
                timer_expires = jiffies;
        }

#if (KERNEL_CENTEVERSION > 415)
	state = inet_sk_state_load(sk);
#else
	state = sk_state_load(sk);
#endif

        if (state == TCP_LISTEN)
                rx_queue = sk->sk_ack_backlog;
        else
                /* Because we don't lock the socket,
                 * we might find a transient negative value.
                 */
                rx_queue = max_t(int, tp->rcv_nxt - tp->copied_seq, 0);

	record_ipv4_route(f, src, srcp, dest, destp, state,
	    tp->snd_cwnd * tp->mss_cache, tp->rcv_wnd, tp->snd_wnd,
	    tp->advmss, tp->bytes_received, tp->segs_in, tp->bytes_acked,
	    tp->segs_out, tp->total_retrans, jiffies_to_clock_t(icsk->icsk_rto),
	    tp->srtt_us, tp->write_seq - tp->pushed_seq,
	    tp->pushed_seq - tp->snd_una);
}


static void get_timewait4_sock(const struct inet_timewait_sock *tw,
                               struct seq_file *f)
{
        __be32 dest, src;
        __u16 destp, srcp;

        dest  = tw->tw_daddr;
        src   = tw->tw_rcv_saddr;
        destp = ntohs(tw->tw_dport);
        srcp  = ntohs(tw->tw_sport);

	record_ipv4_route(f, src, srcp, dest, destp, tw->tw_substate, 0, 0,
	    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

#define TMPSZ 150

static int connstat_seq_show(struct seq_file *seq, void *v)
{
	struct sock *sk = v;

        seq_setwidth(seq, TMPSZ - 1);
        if (v == SEQ_START_TOKEN) {
		seq_puts(seq,
		    "laddr,"
		    "lport,"
		    "raddr,"
		    "rport,"
		    "state,"
		    "inbytes,"
		    "insegs,"
		    "outbytes,"
		    "outsegs,"
		    "retranssegs,"
		    "suna,"
		    "unsent,"
		    "swnd,"
		    "cwnd,"
		    "rwnd,"
		    "mss,"
		    "rto,"
		    "rtt");
                goto out;
        }

	if (sk->sk_state == TCP_TIME_WAIT)
		get_timewait4_sock(v, seq);
	else if (sk->sk_state == TCP_NEW_SYN_RECV)
		get_openreq4(v, seq);
	else
		get_tcp4_sock(v, seq);
out:
        seq_pad(seq, '\n');
        return 0;
}

/*
 * Relies on code in linux/net/ipv4/tcp_ipv4.c to create and register
 * /proc/net/stats_tcp file.  A callback will be made to connstat_seq_show
 * to generate content for the corresponding seq file when needed.
 */
#if (KERNEL_CENTEVERSION > 415)

static const struct seq_operations connstat_seq_ops = {
        .show           = connstat_seq_show,
        .start          = tcp_seq_start,
        .next           = tcp_seq_next,
        .stop           = tcp_seq_stop,
};

static struct tcp_seq_afinfo connstat_seq_afinfo = {
        .family         = AF_INET,
};

static int __net_init connstat_proc_init_net(struct net *net)
{
        if (!proc_create_net_data("stats_tcp", 0444, net->proc_net, &connstat_seq_ops,
                        sizeof(struct tcp_iter_state), &connstat_seq_afinfo))
                return -ENOMEM;
        return 0;
}

static void __net_exit connstat_proc_exit_net(struct net *net)
{
        remove_proc_entry("stats_tcp", net->proc_net);
}

#else

static const struct file_operations connstat_seq_fops = {
        .owner   = THIS_MODULE,
        .open    = tcp_seq_open,
        .read    = seq_read,
        .llseek  = seq_lseek,
        .release = seq_release_net
};

static struct tcp_seq_afinfo connstat_seq_afinfo = {
        .name           = "stats_tcp",
	.family         = AF_INET,
        .seq_fops       = &connstat_seq_fops,
        .seq_ops        = {
                .show           = connstat_seq_show,
        },
};

static int __net_init connstat_proc_init_net(struct net *net)
{
        return tcp_proc_register(net, &connstat_seq_afinfo);
}

static void __net_exit connstat_proc_exit_net(struct net *net)
{
	tcp_proc_unregister(net, &connstat_seq_afinfo);
}

#endif

static struct pernet_operations connstat_net_ops = {
        .init = connstat_proc_init_net,
        .exit = connstat_proc_exit_net,
};


int connstat_init_module(void)
{
	return register_pernet_subsys(&connstat_net_ops);
}

void connstat_cleanup_module(void)
{
	unregister_pernet_subsys(&connstat_net_ops);
}

MODULE_LICENSE("GPL");
module_init(connstat_init_module);
module_exit(connstat_cleanup_module);
