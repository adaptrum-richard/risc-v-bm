#include "ip_neighbor.h"
#include "types.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "slab.h"

static struct neighbor_entry *p_entries = NULL;
struct spinlock neighbor_lock;
void ip_neighbor_init(void)
{
    int i;
    p_entries = kmalloc(sizeof(struct neighbor_entry)*NEIGHBOR_ENTRIES);
    if(!p_entries) {
        pr_err("%s failed\n", __func__);
        return;       
    }
    memset(p_entries, 0, sizeof(struct neighbor_entry)*NEIGHBOR_ENTRIES);

    for(i = 0; i < NEIGHBOR_ENTRIES; i++){
        p_entries[i].time = MAX_NEIGHBOR_TIME;
    }
    initlock(&neighbor_lock, "neighbor");
}

void neighbor_periodic(void)
{
    int i;

    for (i = 0; i < NEIGHBOR_ENTRIES; ++i){
        if (p_entries[i].time < MAX_NEIGHBOR_TIME)
            p_entries[i].time++;
    }
}

void neighbor_add(ipaddr_t ipaddr, struct neighbor_addr *addr)
{
    int i, oldest;
    uint8 oldest_time;
    oldest_time = 0;
    oldest = 0;

    /*找到一个没有使用的项，最旧且使用过的项*/
    acquire(&neighbor_lock);
    for(i = 0; i < NEIGHBOR_ENTRIES; ++i){
        if(ipaddr_cmp(p_entries[i].ipaddr, ipaddr)){
            oldest = i;
            break;
        }
        if(p_entries[i].time == MAX_NEIGHBOR_TIME){
            oldest = i;
            break;
        }
        if(p_entries[i].time > oldest_time){
            oldest = i;
            oldest_time = p_entries[i].time;
        }
    }
    p_entries[oldest].time = 0;
    ipaddr_copy(p_entries[oldest].ipaddr, ipaddr);
    ethaddr_copy(&p_entries[oldest].addr, addr);
    release(&neighbor_lock);
}

static struct neighbor_entry *find_entry(ipaddr_t ipaddr)
{
    int i;
    for(i = 0; i < NEIGHBOR_ENTRIES; ++i){
        if(ipaddr_cmp(p_entries[i].ipaddr, ipaddr)){
            return &p_entries[i];
        }
    }
    return NULL;
}

void neighbor_update(ipaddr_t ipaddr)
{
    struct neighbor_entry *entry;
    acquire(&neighbor_lock);
    entry = find_entry(ipaddr);
    if(entry)
        entry->time = 0;
    release(&neighbor_lock);
}

int neighbor_lookup(ipaddr_t ipaddr, struct neighbor_addr *na)
{
    struct neighbor_entry *entry;
    if(!na){
        pr_err("%s arg error\n", __func__);
        return -1;
    }
    acquire(&neighbor_lock);
    entry = find_entry(ipaddr);
    if(entry){
        ethaddr_copy(&na->addr, &entry->addr);
        release(&neighbor_lock);
        return 0;
    }
    release(&neighbor_lock);
    return -1;
}
