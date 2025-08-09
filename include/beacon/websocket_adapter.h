//
// Created by Henrique on 8/9/2025.
//
#pragma once

#include <string>

namespace beacon {
    class WebSocketAdapter {
    public:
        WebSocketAdapter() = default;

        void connect(const std::string &uri);
    };
} // namespace beacon