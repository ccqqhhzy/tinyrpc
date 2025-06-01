# tinyrpc - A Lightweight C++ RPC Framework

**tinyrpc** 是一个基于 C++14 的轻量级远程过程调用（RPC）框架，支持 Protobuf 和自定义结构体序列化。适用于高性能、低延迟的分布式系统通信场景。

---

## 📌 特性

- **轻量设计**：代码简洁，易于理解和二次开发。
- **多协议支持**：原生支持 Google Protocol Buffers（Protobuf）和自定义结构体（CC）。
- **异步/同步调用**：灵活支持同步请求与异步回调模型。
- **模块化架构**：组件清晰分离，便于扩展和维护。

---

## 🛠️ 构建方式

### 依赖项

- C++14 或以上版本编译器（g++ / clang++）
- [Google Protocol Buffers](https://developers.google.com/protocol-buffers)（如使用 pb 协议）