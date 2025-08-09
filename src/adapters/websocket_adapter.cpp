//
// Created by Henrique on 8/9/2025.
//

#include <beacon/websocket_adapter.h>
#include <iostream>

namespace beacon {
    void WebSocketAdapter::connect(const std::string &uri) {
        std::cout << "Connecting to WebSocket at " << uri << "..." << std::endl;
    }
} // namespace beacon