#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/tcp.h>   //Provides declarations for tcp header
#include <netinet/ip.h>    //Provides declarations for ip header
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <vector>

#include "pcap.h"
#include "../../common/RuntimeNode.h"


void print_data(const u_char* data, int size);
void print_ip_header(const u_char* packet);
void print_tcp_packet(const u_char *packet, int size);
void forward_packet(const u_char* packet, int size);
void process_packet(u_char *arg, const struct pcap_pkthdr* pkthdr,
        const u_char* packet);
// void configure_device_read_handle(std::string packet_filter_expr,
//                                   std::string dev);
void configure_device_read_handles(std::string packet_filter_expr);
void configure_device_write_handle(std::string packet_filter_expr,
                                  std::string dev);

struct Packet {

public:
    const pcap_pkthdr* header;
    const u_char* payload;
}

std::mutex m;

FILE *logfile;
struct sockaddr_in source,dest;
pcap_t* src_dev_handle;
pcap_t* dst_dev_handle;

std::map<std::string, RuntimeNode> interface_leaf_map;
std::map<std::string, pcap_t*> src_dev_handle_map;
std::map<int, std::vector<Packet *> > packet_map;

int main(int argc,char **argv)
{
    /* hard coded stuff for testing */

    RuntimeNode n1 (1, snort);
    RuntimeNode n2 (2, haproxy);

    interface_leaf_map.insert(make_pair("eth1", n1));
    interface_leaf_map.insert(make_pair("eth2", n2));

    /* more info here: https://linux.die.net/man/7/pcap-filter */
    std::string packet_filter_expr = "tcp";
    
    logfile = fopen("log.txt","w");
    if (logfile == NULL) printf("Unable to create file.");

    /* Now get a device */
    // dev = pcap_lookupdev(errbuf);
    
    // std::string src_dev = "eth1";
    std::string dst_dev = "eth2";
    
    // configure_device_read_handle(packet_filter_expr, src_dev);
    configure_device_read_handles(packet_filter_expr);
    configure_device_write_handle(packet_filter_expr, dst_dev);
    
    /* loop for callback function */
    // pcap_loop(src_dev_handle, -1, process_packet, NULL);
    for (std::map<std::string, RuntimeNode>::iterator it = mymap.begin(); it != mymap.end(); ++it) {
        /* loop for callback function */
        std::cout << "looping\n";
        pcap_loop(src_dev_handle, 3, process_packet, NULL);
    }
    return 0;
}

void configure_device_read_handles(std::string packet_filter_expr)
{
    for (std::map<std::string, RuntimeNode>::iterator it = mymap.begin(); it != mymap.end(); ++it) {
        
        std::string dev = it->first;
        char errbuf[PCAP_ERRBUF_SIZE];
        const u_char *packet;
        struct bpf_program fp;        /* hold compiled program */
        bpf_u_int32 maskp;            /* subnet mask */
        bpf_u_int32 netp;             /* ip */

        if (pcap_lookupnet(dev.c_str(), &netp, &maskp, errbuf) < 0) {
            std::cerr << errbuf << std::endl;
            exit(-1);
        }

        /* open device for reading in promiscuous mode */
        src_dev_handle_map[dev] = pcap_open_live(dev.c_str(), BUFSIZ, 1,-1, errbuf);

        if(src_dev_handle_map[dev] == NULL) {
            printf("pcap_open_live(): %s\n", errbuf);
            exit(-1);
        }

        /* Now we'll compile the filter expression*/
        if(pcap_compile(src_dev_handle_map[dev], &fp, packet_filter_expr.c_str(), 0, netp) == -1) {
            fprintf(stderr, "Error calling pcap_compile\n");
            exit(-1);
        }
        
        /* set the filter */
        if(pcap_setfilter(src_dev_handle_map[dev], &fp) == -1) {
            fprintf(stderr, "Error setting filter\n");
            exit(-1);
        }

    }
}

// void configure_device_read_handle(std::string packet_filter_expr,
//                                   std::string dev)
// {
//     char errbuf[PCAP_ERRBUF_SIZE];
//     const u_char *packet;
//     struct bpf_program fp;        /* hold compiled program */
//     bpf_u_int32 maskp;            /* subnet mask */
//     bpf_u_int32 netp;             /* ip */

//     if (pcap_lookupnet(dev.c_str(), &netp, &maskp, errbuf) < 0) {
//         std::cerr << errbuf << std::endl;
//         exit(-1);
//     }

 
//     /* open device for reading in promiscuous mode */
//     src_dev_handle = pcap_open_live(dev.c_str(), BUFSIZ, 1,-1, errbuf);

//     if(src_dev_handle == NULL) {
//         printf("pcap_open_live(): %s\n", errbuf);
//         exit(-1);
//     }
    
//     /* Now we'll compile the filter expression*/
//     if(pcap_compile(src_dev_handle, &fp, packet_filter_expr.c_str(), 0, netp) == -1) {
//         fprintf(stderr, "Error calling pcap_compile\n");
//         exit(-1);
//     }
    
//     /* set the filter */
//     if(pcap_setfilter(src_dev_handle, &fp) == -1) {
//         fprintf(stderr, "Error setting filter\n");
//         exit(-1);
//     }
// }

void configure_device_write_handle(std::string packet_filter_expr,
                                  std::string dev)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    const u_char *packet;
    struct bpf_program fp;        /* hold compiled program */
    bpf_u_int32 maskp;            /* subnet mask */
    bpf_u_int32 netp;             /* ip */

    if (pcap_lookupnet(dev.c_str(), &netp, &maskp, errbuf) < 0) {
        std::cerr << errbuf << std::endl;
    }

    
    /* open device for reading in promiscuous mode */
    dst_dev_handle = pcap_open_live(dev.c_str(), BUFSIZ, 1,-1, errbuf);

    if(dst_dev_handle == NULL) {
        printf("pcap_open_live(): %s\n", errbuf);
        exit(-1);
    }
}

void process_packet(u_char *arg,
                    const struct pcap_pkthdr* pkthdr,
                    const u_char* packet)
{
    std::cout << "received tcp packet\n";
    
    /* get the packet id */
    unsigned short iphdrlen;
         
    struct iphdr *iph = (struct iphdr *)packet;
    iphdrlen = iph->ihl*4;
     
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;
     
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;
    
    int packet_id = ntohs(iph->id);

    /* add packet to the map */
    m.lock();
    std::vector<Packet> *pkts;
    if (packet_map.count(packet_id) > 0) {
        std::vector<Packet> *pkts = packet_map[packet_id];
    } else {
        std::vector<Packet> *pkts = new std::vector<Packet>();
    }
    Packet p;
    p.header = pkthdr;
    p.payload = packet;
    pkts->push_back(p);
    packet_map[packet_id] = pkts;
    m.unlock();

    // print_tcp_packet(packet, pkthdr->len);
    // forward_packet(packet, pkthdr->len);
}

void print_tcp_packet(const u_char *packet, int size)
{

    unsigned short iphdrlen;
     
    struct iphdr *iph = (struct iphdr *)packet;
    iphdrlen = iph->ihl*4;
    std::cout << size << " " << iphdrlen << std::endl;
    struct tcphdr *tcph=(struct tcphdr*)(packet + iphdrlen);
             
    fprintf(logfile,"\n\n***********************TCP Packet*************************\n");    
         
    print_ip_header(packet);
         
    fprintf(logfile,"\n");
    fprintf(logfile,"TCP Header\n");
    fprintf(logfile,"   |-Source Port      : %u\n",ntohs(tcph->source));
    fprintf(logfile,"   |-Destination Port : %u\n",ntohs(tcph->dest));
    fprintf(logfile,"   |-Sequence Number    : %u\n",ntohl(tcph->seq));
    fprintf(logfile,"   |-Acknowledge Number : %u\n",ntohl(tcph->ack_seq));
    fprintf(logfile,"   |-Header Length      : %d DWORDS or %d BYTES\n" ,(unsigned int)tcph->doff,(unsigned int)tcph->doff*4);
    //fprintf(logfile,"   |-CWR Flag : %d\n",(unsigned int)tcph->cwr);
    //fprintf(logfile,"   |-ECN Flag : %d\n",(unsigned int)tcph->ece);
    fprintf(logfile,"   |-Urgent Flag          : %d\n",(unsigned int)tcph->urg);
    fprintf(logfile,"   |-Acknowledgement Flag : %d\n",(unsigned int)tcph->ack);
    fprintf(logfile,"   |-Push Flag            : %d\n",(unsigned int)tcph->psh);
    fprintf(logfile,"   |-Reset Flag           : %d\n",(unsigned int)tcph->rst);
    fprintf(logfile,"   |-Synchronise Flag     : %d\n",(unsigned int)tcph->syn);
    fprintf(logfile,"   |-Finish Flag          : %d\n",(unsigned int)tcph->fin);
    fprintf(logfile,"   |-Window         : %d\n",ntohs(tcph->window));
    fprintf(logfile,"   |-Checksum       : %d\n",ntohs(tcph->check));
    fprintf(logfile,"   |-Urgent Pointer : %d\n",tcph->urg_ptr);
    fprintf(logfile,"\n");
    fprintf(logfile,"                        DATA Dump                         ");
    fprintf(logfile,"\n");
         
    fprintf(logfile,"IP Header\n");
    print_data(packet,iphdrlen);
         
    fprintf(logfile,"TCP Header\n");
    print_data(packet + iphdrlen,tcph->doff*4);
         
    fprintf(logfile,"Data Payload\n");  
    print_data(packet + iphdrlen + tcph->doff*4 , (size - tcph->doff*4-iph->ihl*4) );
                         
    fprintf(logfile,"\n###########################################################\n");
}

void print_ip_header(const u_char* packet)
{
    unsigned short iphdrlen;
         
    struct iphdr *iph = (struct iphdr *)packet;
    iphdrlen = iph->ihl*4;
     
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;
     
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;
     
    fprintf(logfile,"\n");
    fprintf(logfile,"IP Header\n");
    fprintf(logfile,"   |-IP Version        : %d\n",(unsigned int)iph->version);
    fprintf(logfile,"   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
    fprintf(logfile,"   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
    fprintf(logfile,"   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(iph->tot_len));
    fprintf(logfile,"   |-Identification    : %d\n",ntohs(iph->id));
    //fprintf(logfile,"   |-Reserved ZERO Field   : %d\n",(unsigned int)iphdr->ip_reserved_zero);
    //fprintf(logfile,"   |-Dont Fragment Field   : %d\n",(unsigned int)iphdr->ip_dont_fragment);
    //fprintf(logfile,"   |-More Fragment Field   : %d\n",(unsigned int)iphdr->ip_more_fragment);
    fprintf(logfile,"   |-TTL      : %d\n",(unsigned int)iph->ttl);
    fprintf(logfile,"   |-Protocol : %d\n",(unsigned int)iph->protocol);
    fprintf(logfile,"   |-Checksum : %d\n",ntohs(iph->check));
    fprintf(logfile,"   |-Source IP        : %s\n",inet_ntoa(source.sin_addr));
    fprintf(logfile,"   |-Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
}

void print_data(const u_char* data, int size)
{
     
    for(int i=0; i < size; i++)
    {
        if(i != 0 && i % 16 == 0)   //if one line of hex printing is complete...
        {
            fprintf(logfile,"         ");
            for(int j = i - 16; j < i; j++)
            {
                if(data[j]>=32 && data[j]<=128)
                    fprintf(logfile,"%c",(unsigned char)data[j]); //if its a number or alphabet
                 
                else fprintf(logfile,"."); //otherwise print a dot
            }
            fprintf(logfile,"\n");
        } 
         
        if(i % 16 == 0) fprintf(logfile,"   ");
            fprintf(logfile," %02X",(unsigned int)data[i]);
                 
        if( i==size-1)  //print the last spaces
        {
            for(int j=0;j<15-i%16;j++) fprintf(logfile,"   "); //extra spaces
             
            fprintf(logfile,"         ");
             
            for(int j=i-i%16 ; j<=i ; j++)
            {
                if(data[j]>=32 && data[j]<=128) fprintf(logfile,"%c",(unsigned char)data[j]);
                else fprintf(logfile,".");
            }
            fprintf(logfile,"\n");
        }
    }
}

void forward_packet(const u_char* packet, int size)
{
    if (pcap_sendpacket(dst_dev_handle, packet, size) < 0) {
        std::cerr << strerror(errno) << std::endl;
    }
}