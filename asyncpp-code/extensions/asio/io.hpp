#pragma once
#ifndef INC_ASIO_IO_HPP_
#define INC_ASIO_IO_HPP_
#include "async-promise/promise.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
namespace promise{
template<typename RESULT>
inline void setPromise(Defer defer,
    boost::system::error_code err,
    const char *errorString,
    const RESULT &result) {
    if (err) {
        std::cerr << errorString << ": " << err.message() << "\n";
        defer.reject(err);
    }
    else
        defer.resolve(result);
}
template<typename Resolver>
inline Promise async_resolve(
    Resolver &resolver,
    const std::string &host, const std::string &port) {
    return newPromise([&](Defer &defer) {
        resolver.async_resolve(
            host,
            port,
            [defer](boost::system::error_code err,
                typename Resolver::results_type results) {
                setPromise(defer, err, "resolve", results);
        });
    });
}
template<typename ResolverResult, typename Socket>
inline Promise async_connect(
    Socket &socket,
    const ResolverResult &results) {
    return newPromise([&](Defer &defer) {
        boost::asio::async_connect(
            socket,
            results.begin(),
            results.end(),
            [defer](boost::system::error_code err,
                typename ResolverResult::iterator i) {
                setPromise(defer, err, "connect", i);
        });
    });
}
template<typename Stream, typename Buffer, typename Content>
inline Promise async_read(Stream &stream,
    Buffer &buffer,
    Content &content) {
    return newPromise([&](Defer &defer) {
        boost::beast::http::async_read(stream, buffer, content,
            [defer](boost::system::error_code err,
                std::size_t bytes_transferred) {
                setPromise(defer, err, "read", bytes_transferred);
        });
    });
}
template<typename Stream, typename Content>
inline Promise async_write(Stream &stream, Content &content) {
    return newPromise([&](Defer &defer) {
        boost::beast::http::async_write(stream, content,
            [defer](boost::system::error_code err,
                std::size_t bytes_transferred) {
                setPromise(defer, err, "write", bytes_transferred);
        });
    });
}
}
#endif