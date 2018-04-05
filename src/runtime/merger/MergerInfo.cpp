//
// Created by Victoria on 2018-03-30.
//

#include "MergerInfo.h"



MergerInfo::MergerInfo(std::map<std::string, int> port_to_node_map,
                       std::vector<ConflictItem*> conflicts_list,
                       std::map<int, RuntimeNode*> node_map) {
    this->port_to_node_map = port_to_node_map;
    this->conflicts_list = conflicts_list;
    this->node_map = node_map;
}

std::map<std::string, int> MergerInfo::get_port_to_node_map() {
    return port_to_node_map;
};

std::vector<ConflictItem*> MergerInfo::get_conflicts_list() {
    return conflicts_list;
};

std::map<int, RuntimeNode*> MergerInfo::get_node_map() {
    return node_map;
}


/**
 *
 * @return A pointer to a MergerInfo object with dummy data for testing purposes
 */
MergerInfo* MergerInfo::get_dummy_merger_info() {
    std::map<std::string, int> port_to_node_map;
    std::vector<ConflictItem*> conflicts_list;
    std::map<int, RuntimeNode*> node_map;

    MergerInfo* mi = new MergerInfo(port_to_node_map, conflicts_list, node_map);
    return mi;
}
