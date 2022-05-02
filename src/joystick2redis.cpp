#include <Config.h>
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
  DynamicJsonDocument config(10240);
  loadConfig(config.to<JsonObject>(), argc, argv);
  Thread workerThread("worker");
  Joystick joystick(workerThread);  // blocking thread
  Redis redis(workerThread, config["redis"].as<JsonObject>());
  redis.connect();

  std::string dstPrefix = "dst/joystick/";
  std::string srcPrefix = "src/joystick/";

  JsonObject joystickConfig = config["joystick"];

  joystick.config(joystickConfig);
  joystick.init();
  joystick.connect();

  joystick.axisEvent >> [srcPrefix, &redis](const AxisEvent &ae) {
    std::string topic = srcPrefix + "axis/" + std::to_string(ae.axis);
    std::string message = std::to_string(ae.value);
    redis.publish(topic, message);
  };

  joystick.buttonEvent >> [srcPrefix, &redis](const ButtonEvent &ae) {
    std::string topic = srcPrefix + "button/" + std::to_string(ae.button);
    std::string message = std::to_string(ae.value);
    redis.publish(topic, message);
  };

  redis.response() >> [&](const Json &) {

  };
  workerThread.run();
}

void deepMerge(JsonVariant dst, JsonVariant src)
{
    if (src.is<JsonObject>())
    {
        for (auto kvp : src.as<JsonObject>())
        {
            deepMerge(dst.getOrAddMember(kvp.key()), kvp.value());
        }
    }
    else
    {
        dst.set(src);
    }
}

bool loadConfig(JsonObject cfg, int argc, char **argv)
{
    // override args
    int c;
    while ((c = getopt(argc, argv, "h:p:s:b:f:t:v")) != -1)
        switch (c)
        {
        case 'f':
        {
            std::string s = loadFile(optarg);
            DynamicJsonDocument doc(10240);
            deserializeJson(doc, s);
            deepMerge(cfg, doc);
            break;
        }
        case 'h':
            cfg["redis"]["host"] = optarg;
            break;
        case 'p':
            cfg["redis"]["port"] = atoi(optarg);
            break;
        case 'v':
        {
            logger.setLevel(Log::L_DEBUG);
            break;
        }
        case '?':
            printf("Usage %s -v -f <configFile.json>",
                   argv[0]);
            break;
        default:
            WARN("Usage %s -v -f <configFile.json> ",
                 argv[0]);
            abort();
        }
    std::string s;
    serializeJson(cfg, s);
    INFO("config:%s", s.c_str());
    return true;
};
