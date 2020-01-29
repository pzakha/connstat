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
#include <linux/inet.h>
#include <linux/proc_fs.h>

struct connstat_data {
	__be32 laddr;
	__be32 raddr;
	__u16 lport;
	__u16 rport;
	int state;
	u32 cwnd;
	u32 rwnd;
	u32 swnd;
	u32 mss;
	u32 insegs;
	u32 outsegs;
	u32 retranssegs;
	u32 rto;
	u32 rtt;
	u32 suna;
	u32 unsent;
	u32 rxqueue;
	u64 inbytes;
	u64 outbytes;
};

#define GET_OCTET0(hip) (hip & 0xFF)
#define GET_OCTET1(hip) ((hip >> 8) & 0xFF)
#define GET_OCTET2(hip) ((hip >> 16) & 0xFF)
#define GET_OCTET3(hip) (hip >> 24)

static char *tcp_state_strings[] = {
	"NONE",	     "ESTABLISHED", "SYN_SENT",	    "SYN_RECEIVED", "FIN_WAIT1",
	"FIN_WAIT2", "TIME_WAIT",   "CLOSED",	    "CLOSE_WAIT",   "LAST_ACK",
	"LISTEN",    "CLOSING",	    "NEW_SYN_RECV", "MAX_STATES"
};

/* Print an IPv4 address into the provided string and return that string. */
static char *ipv4_ntop(__be32 addr, char *addrstr)
{
	snprintf(addrstr, INET_ADDRSTRLEN, "%d.%d.%d.%d", GET_OCTET0(addr),
		 GET_OCTET1(addr), GET_OCTET2(addr), GET_OCTET3(addr));
	return addrstr;
}

static void record_ipv4_conn(struct seq_file *f, struct connstat_data *data)
{
	char laddr[INET_ADDRSTRLEN], raddr[INET_ADDRSTRLEN];

	seq_printf(f,
		   "%s,%u,%s,%u,%s,%llu,%u,%llu,%u,%u,"
		   "%u,%u,%u,%u,%u,%u,%u,%u,%u",
		   ipv4_ntop(data->laddr, laddr), data->lport,
		   ipv4_ntop(data->raddr, raddr), data->rport,
		   tcp_state_strings[data->state], data->inbytes, data->insegs,
		   data->outbytes, data->outsegs, data->retranssegs, data->suna,
		   data->unsent, data->swnd, data->cwnd, data->rwnd, data->mss,
		   data->rto, data->rtt, data->rxqueue);
}

static void get_openreq4(const struct request_sock *req, struct seq_file *f)
{
	const struct inet_request_sock *ireq = inet_rsk(req);
	struct connstat_data data = { 0 };

	data.laddr = ireq->ir_loc_addr;
	data.raddr = ireq->ir_rmt_addr;
	data.lport = ireq->ir_num;
	data.rport = ntohs(ireq->ir_rmt_port);
	data.state = TCP_SYN_RECV;

	record_ipv4_conn(f, &data);
}

static void get_tcp4_sock(struct sock *sk, struct seq_file *f)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	const struct inet_connection_sock *icsk = inet_csk(sk);
	const struct inet_sock *inet = inet_sk(sk);
	struct connstat_data data = { 0 };

	data.laddr = inet->inet_rcv_saddr;
	data.raddr = inet->inet_daddr;
	data.lport = ntohs(inet->inet_sport);
	data.rport = ntohs(inet->inet_dport);
#if (KERNEL_CENTEVERSION > 415)
	data.state = inet_sk_state_load(sk);
#else
	data.state = sk_state_load(sk);
#endif
	data.cwnd = tp->snd_cwnd * tp->mss_cache;
	data.rwnd = tp->rcv_wnd;
	data.swnd = tp->snd_wnd;
	data.mss = tp->advmss;
	data.insegs = tp->segs_in;
	data.outsegs = tp->segs_out;
	data.retranssegs = tp->total_retrans;
	data.rto = jiffies_to_clock_t(icsk->icsk_rto);
	data.rtt = tp->srtt_us;
	data.suna = tp->snd_nxt - tp->snd_una;
	data.unsent = tp->write_seq - tp->snd_nxt;
	data.inbytes = tp->bytes_received;
	data.outbytes = tp->bytes_acked;

	if (data.state == TCP_LISTEN) {
		data.rxqueue = sk->sk_ack_backlog;
	} else {
		/* Because we don't lock the socket,
		 * we might find a transient negative value.
		 */
		data.rxqueue = max_t(int, tp->rcv_nxt - tp->copied_seq, 0);
	}

	record_ipv4_conn(f, &data);
}

static void get_timewait4_sock(const struct inet_timewait_sock *tw,
			       struct seq_file *f)
{
	struct connstat_data data = { 0 };

	data.laddr = tw->tw_rcv_saddr;
	data.raddr = tw->tw_daddr;
	data.lport = ntohs(tw->tw_sport);
	data.rport = ntohs(tw->tw_dport);
	data.state = tw->tw_substate;

	record_ipv4_conn(f, &data);
}

static int connstat_seq_show(struct seq_file *seq, void *v)
{
	struct sock *sk = v;

	if (v == SEQ_START_TOKEN) {
		seq_puts(seq, "laddr,"
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
			      "rtt,"
			      "rxqueue");
		goto out;
	}

	if (sk->sk_state == TCP_TIME_WAIT)
		get_timewait4_sock(v, seq);
	else if (sk->sk_state == TCP_NEW_SYN_RECV)
		get_openreq4(v, seq);
	else
		get_tcp4_sock(v, seq);
out:
	return 0;
}

/*
 * Relies on code in linux/net/ipv4/tcp_ipv4.c to create and register
 * /proc/net/stats_tcp file.  A callback will be made to connstat_seq_show
 * to generate content for the corresponding seq file when needed.
 */
#if (KERNEL_CENTEVERSION > 415)

static const struct seq_operations connstat_seq_ops = {
	.show = connstat_seq_show,
	.start = tcp_seq_start,
	.next = tcp_seq_next,
	.stop = tcp_seq_stop,
};

static struct tcp_seq_afinfo connstat_seq_afinfo = {
	.family = AF_INET,
};

static int __net_init connstat_proc_init_net(struct net *net)
{
	if (!proc_create_net_data(
		    "stats_tcp", 0444, net->proc_net, &connstat_seq_ops,
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
	.owner = THIS_MODULE,
	.open = tcp_seq_open,
	.read = seq_read,
	.llseek = seq_lseek,
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
