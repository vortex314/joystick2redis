#include <Config.h>
#include <ConfigFile.h>
#include <Hardware.h>
#include <Joystick.h>
#include <Log.h>
#include <limero.h>
#include <stdio.h>
#include <unistd.h>

#include "Redis.h"
Log logger;

std::string loadFile(const char *name);
bool loadConfig(JsonObject cfg, int argc, char **argv);

int main(int argc, char **argv) {
  INFO("Loading configuration.");
  Json config;
  config["redis"]["host"] = "localhost";
  config["redis"]["port"] = 6379;
  configurator(config, argc, argv);
  Thread workerThread("worker");
  Joystick joystick(workerThread);  // blocking thread
  Redis redis(workerThread, config["redis"].as<JsonObject>());

  JsonObject joystickConfig = config["joystick"];
  std::string srcPrefix = config["redis"]["topic"] | "src/joystick/";
  redis.connect();
  joystick.config(joystickConfig);
  joystick.init();
  joystick.connect();

  joystick.axisEvent >> [srcPrefix, &redis](const AxisEvent &ae) {
    std::string topic = srcPrefix + "axis/" + std::to_string(ae.axis);
    std::string message = std::to_string(ae.value);
    redis.publish(topic, message);
  };

  joystick.buttonEvent >> [srcPrefix, &redis](const ButtonEvent &be) {
    std::string topic = srcPrefix + "button/" + std::to_string(be.button);
    std::string message = std::to_string(be.value);
    redis.publish(topic, message);
  };

  redis.response() >> [&](const Json &) {
    // ignore responses from redis
  };
  workerThread.run();
}
