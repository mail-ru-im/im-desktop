#include "stdafx.h"

#include "network_change_observer.h"

#include "../curl_handler.h"

namespace core {
    network_change_observer::network_change_observer() = default;
    network_change_observer::~network_change_observer() = default;
    void network_change_observer::on_network_down()
    {
        // nothing to do
    }
    void network_change_observer::on_network_up()
    {
        curl_handler::instance().reset_sockets();
    }
    void network_change_observer::on_network_change() 
    {
        curl_handler::instance().reset_sockets();
    }
}
