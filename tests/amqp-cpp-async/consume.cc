//
// Created by denn nevera on 2019-06-19.
//

#include <uv.h>

#include "amqpcpp.h"
#include "amqp_tcp_socket.h"
#include "amqpcpp/libuv.h"

#include "capy/amqp.h"
#include "gtest/gtest.h"

/**
 *  Custom handler
 */
class MyHandler : public AMQP::LibUvHandler
{
private:
    /**
     *  Method that is called when a connection error occurs
     *  @param  connection
     *  @param  message
     */
    virtual void onError(AMQP::TcpConnection *connection, const char *message) override
    {
      std::cout << "error: " << message << std::endl;
    }

    /**
     *  Method that is called when the TCP connection ends up in a connected state
     *  @param  connection  The TCP connection
     */
    virtual void onConnected(AMQP::TcpConnection *connection) override
    {
      std::cout << "connected" << std::endl;
    }

public:
    /**
     *  Constructor
     *  @param  uv_loop
     */
    MyHandler(uv_loop_t *loop) : AMQP::LibUvHandler(loop) {}

    /**
     *  Destructor
     */
    virtual ~MyHandler() = default;
};


TEST(Initial, InitialTest) {

  // access to the event loop
  auto *loop = uv_default_loop();

  // handler for libev
  MyHandler handler(loop);

  // make a connection
  AMQP::TcpConnection connection(&handler, AMQP::Address("amqp://guest:guest@localhost/"));

  // we need a channel too
  AMQP::TcpChannel channel(&connection);

  // create a queue
  channel

          .declareQueue("capy-test", AMQP::durable)

          .onSuccess([&connection](const std::string &name, uint32_t messagecount, uint32_t consumercount) {

              // report the name of the temporary queue
              std::cout << "declared queue " << name << std::endl;
          })

          .onError([](const char *message){

              std::cerr << "declared queue error" << message << std::endl;

          });


  channel

          .bindQueue("amq.topic", "capy-test", "echo.ping")

          .onSuccess([](){

              std::cout << "bind operation succeed..." << std::endl;

          })

          .onError([](const char *message) {

              std::cout << "bind operation failed: " << message << std::endl;
          });


  channel

          .consume("capy-test")

          .onReceived([&channel, &loop](
                  const AMQP::Message &message,
                  uint64_t deliveryTag,
                  bool redelivered){

              std::cout << "message received: " << message.body() << " | "<< message.bodySize() << std::endl;

              channel.ack(deliveryTag);

              uv_stop(loop);

          })

          .onSuccess( [](const std::string &consumertag) {

              std::cout << "consume operation started: " << consumertag << std::endl;
          })

          .onError([](const char *message) {

              std::cout << "consume operation failed: " << message << std::endl;
          });


  // run the loop
  uv_run(loop, UV_RUN_DEFAULT);
}