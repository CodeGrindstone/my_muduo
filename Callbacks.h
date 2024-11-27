#pragma once

#include <memory>
#include <functional>

class Buffer;
class Timestamp;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using closeCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;


using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)>;