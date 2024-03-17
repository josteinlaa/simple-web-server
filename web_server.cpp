#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>

using namespace std;
using namespace boost::asio::ip;
namespace http = boost::beast::http;

class SimpleWebServer {
private:
  class Connection : public std::enable_shared_from_this<Connection> {
  public:
    tcp::socket socket;
    Connection(boost::asio::io_service &io_service) : socket(io_service) {}
    void start() {
      read_request();
    }

  private:
    boost::beast::flat_buffer buffer;
    http::request<http::string_body> request;
    http::response<http::string_body> response;
    void read_request() {

      auto self = shared_from_this();

      http::async_read(socket,
                       buffer,
                       request,
                       [self](boost::beast::error_code ec,
                              std::size_t bytes_transferred) {
                         boost::ignore_unused(bytes_transferred);
                         if (!ec)
                           self->prepare_response();
                       });
    }
    void prepare_response() {
      response.version(request.version());
      response.set(http::field::content_type, "text/plain");

      if (request.target() == "/") {
        response.result(http::status::ok);
        response.body() = "Dette er hovedsiden";
      } else if (request.target() == "/en_side") {
        response.result(http::status::ok);
        response.body() = "Dette er en side";
      } else {
        response.result(http::status::not_found);
        response.body() = "404 Not Found";
      }
      response.prepare_payload();
      write_response();
    }
    void write_response() {

      auto self = shared_from_this();

      http::async_write(socket,
                        response,
                        [self](boost::beast::error_code ec, std::size_t) {
                          self->socket.shutdown(tcp::socket::shutdown_send, ec);
                        });
    }
  };

  boost::asio::io_service io_service;

  tcp::endpoint endpoint;
  tcp::acceptor acceptor;

  void handle_request(shared_ptr<Connection> connection) {
    connection->start();
  }

  void accept() {
    // The (client) connection is added to the lambda parameter and handle_request
    // in order to keep the object alive for as long as it is needed.
    auto connection = make_shared<Connection>(io_service);

    // Accepts a new (client) connection. On connection, immediately start accepting a new connection
    acceptor.async_accept(connection->socket, [this, connection](const boost::system::error_code &ec) {
      accept();
      // If not error:
      if (!ec) {
        handle_request(connection);
      }
    });
  }

public:
  SimpleWebServer() : endpoint(tcp::v4(), 8080), acceptor(io_service, endpoint) {}

  void start() {
    accept();

    io_service.run();
  }
};

int main() {
  SimpleWebServer web_server;

  cout << "Starting web server" << endl;

  web_server.start();
}
