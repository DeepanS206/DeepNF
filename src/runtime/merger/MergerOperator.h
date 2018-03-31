//
// Created by Victoria on 2018-03-31.
//

#ifndef DEEPNF_MERGEROPERATOR_H
#define DEEPNF_MERGEROPERATOR_H

#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <vector>

#include "packet.h"
#include "pcap.h"
#include "RuntimeNode.h"
#include "MergerInfo.h"
#include "NF.h"

class MergerOperator {

    public:
        MergerOperator();

        // runs the merger
        void run();


    private:
        struct nf_packet {
            public:
                struct packet *pkt;
                NF nf;
        };

        FILE *logfile;
        pcap_t* dst_dev_handle;

        // contains information about virtual network interfaces, conflicting NF pairs, etc.
        MergerInfo* merger_info;

        // maps name of virtual interface to its corresponding RuntimeNode
        std::map<std::string, RuntimeNode> interface_leaf_map;
        std::map<std::string, pcap_t*> src_dev_handle_map;

        // maps packet IDs with list of received packets with that ID
        std::map<int, std::vector<nf_packet>*> packet_map;
        std::string cur_dev;


        /* FUNCTIONS FOR PERFORMING MERGER OPERATIONS */
        // sets up hardcoded MergerInfo object to do testing on
        MergerInfo* setup_dummy_info();


        /* HELPER FUNCTIONS FOR OPERATING ON PACKETS */
        void configure_device_read_handles(std::string packet_filter_expr);
        void configure_device_write_handle(std::string packet_filter_expr,
                                       std::string dev);



};


#endif //DEEPNF_MERGEROPERATOR_H
