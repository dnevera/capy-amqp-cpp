//
// Created by denn nevera on 2019-06-06.
//


#include "broker_constructor.h"
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

TEST(Exchange, ListenTest) {

  auto listener_q  = capy::dispatchq::Queue(1);
  auto publisher_q = capy::dispatchq::Queue(1);

  listener_q.async([] {

      std::cout << "listener thread" << std::endl;

      if (auto broker = create_broker()) {

        std::cout << "listener broker created ... " << std::endl;

        int counter = 0;
        broker->listen(
                "capy-test",
                {"something.*"},
                [&](const capy::amqp::Request& request, capy::amqp::Replay& replay)
                {

                    if (!request) {
                      std::cerr << " listen error: " << request.error().value() << "/" << request.error().message() << std::endl;
                    }
                    else {
                      std::cout << " listen["<< counter << "] received ["<< request->routing_key << "]: " << request->message.dump(4) << std::endl;
                      replay.value() = {"reply", true, counter};
                      counter++;
                    }
                });
      };

  });


  publisher_q.async([]{

      int max_count = 30;

      std::cout << "producer thread" << std::endl;

      if (auto broker = create_broker()) {
        std::cout << "producer broker created ... " << std::endl;

        for (int i = 0; i < max_count ; ++i) {

          std::this_thread::sleep_for(std::chrono::microseconds(10));

          std::string timestamp = std::to_string(time(0));

          capy::json action = {
                  {"action", "someMethodSouldBeExecuted"},
                  {"payload", {"ids", timestamp}, {"timestamp", timestamp}, {"i", i}}
          };

          std::cout << "fetch[" << i << "] action: " <<  action.dump(4) << std::endl;

          std::string key = "something.find";

          key.append(std::to_string(i));

          if (auto error = broker->fetch(action, key, [&](const capy::Result<capy::json> &message){


              if (!message){

                std::cerr << "amqp broker fetch receiving error: " << message.error().value() << " / " << message.error().message()
                          << std::endl;

              }
              else {
                std::cout << "fetch["<< i << "] received: " <<  message->dump(4) << std::endl;
              }


          })) {

            std::cerr << "amqp broker fetch error: " << error.value() << " / " << error.message()
                      << std::endl;

          }

          if (max_count - 1 == i) {
            ::exit(0);
          }

        }

      }

  });

  std::cout << "main thread" << std::endl;

  capy::dispatchq::main::loop::run();

}