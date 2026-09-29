/*
 * eBPF/XDP DDoS Protection - issue #137
 *
 * Per-source-IP packet counting with temporary blocking.
 * Configuration is supplied through the config BPF map.
 */

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <bpf/bpf_helpers.h>

struct config_struct {
    __u32 rate_limit_pps;
    __u32 burst_limit_pps;
    __u32 block_duration_sec;
    __u32 enabled;
};

struct {
    __uint(type, BPF_MAP_TYPE_LRU_HASH);
    __uint(max_entries, 10000);
    __type(key, __u32);
    __type(value, __u64);
} ip_counts SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10000);
    __type(key, __u32);
    __type(value, __u64);
} blocked_ips SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, struct config_struct);
} config SEC(".maps");

SEC("xdp")
int xdp_ddos_filter(struct xdp_md *md)
{
    void *data = (void *)(long)md->data;
    void *data_end = (void *)(long)md->data_end;

    struct ethhdr *eth = data;

    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;

    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;

    struct iphdr *ip = data + sizeof(struct ethhdr);

    if ((void *)(ip + 1) > data_end)
        return XDP_PASS;

    __u32 src = ip->saddr;
    __u32 cfg_key = 0;

    struct config_struct *cfg =
        bpf_map_lookup_elem(&config, &cfg_key);

    if (!cfg || !cfg->enabled)
        return XDP_PASS;

    __u64 now = bpf_ktime_get_ns() / 1000000000ULL;

    __u64 *blocked_until =
        bpf_map_lookup_elem(&blocked_ips, &src);

    if (blocked_until && *blocked_until > now)
        return XDP_DROP;

    if (blocked_until && *blocked_until <= now)
        bpf_map_delete_elem(&blocked_ips, &src);

    __u64 *count =
        bpf_map_lookup_elem(&ip_counts, &src);

    __u64 current_count = count ? *count : 0;
    current_count++;

    bpf_map_update_elem(
        &ip_counts,
        &src,
        &current_count,
        BPF_ANY
    );

    __u32 limit = cfg->burst_limit_pps;

    if (limit > 0 && current_count > limit) {
        __u64 blocked_until_value =
            now + cfg->block_duration_sec;

        bpf_map_update_elem(
            &blocked_ips,
            &src,
            &blocked_until_value,
            BPF_ANY
        );

        return XDP_DROP;
    }

    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
