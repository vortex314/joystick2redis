#include <Joystick.h>

Joystick::Joystick(Thread &thread, const char *name)
    : Actor(thread),
      buttonEvent(20, "buttonEvent"),
      axisEvent(20, "axisEvent"),
      deviceLookupTimer(thread, 2000, true, "deviceLookup"),
      devicePollTimer(thread, 20, true, "devicePoll") {
  buttonEvent.async(thread);
  axisEvent.async(thread);
}

bool Joystick::config(JsonObject &config) {
  _device = config["device"] | "/dev/input/js0";
  return true;
}

bool Joystick::init() {
  deviceLookupTimer >> [&](const TimerMsg &tm) {
    if (exists() && connected()) return;
    if (exists() && !connected()) {
      connect();
    } else if (!exists() && connected()) {
      disconnect();
    } else if (!exists() && !connected()) {
      INFO(" waiting for device %s", _device.c_str());
    }
  };

  return true;
}

void Joystick::emitEvent() {
  while (true) {
    Joystick::EventClass cls;
    int instance;
    int value;
    if (getEvent(cls, instance, value)) break;
    if (cls == EV_AXIS) {
      axisEvent.on({instance, value});
    } else if (cls == EV_BUTTON) {
      buttonEvent.on({instance, value});
    } else {
      INFO(" unknown event ");
    }
  }
}

void Joystick::onReadFd(void *arg) {
  Joystick *joystick = (Joystick *)arg;
  joystick->emitEvent();
}

int Joystick::connect() {
  INFO("Connecting to '%s' ....", _device.c_str());
  _fd = ::open(_device.c_str(), O_EXCL | O_RDWR | O_NOCTTY | O_NDELAY);

  if (_fd == -1) {
    ERROR("connect: Unable to open '%s' errno : %d : %s ", _device.c_str(),
          errno, strerror(errno));
    _fd = -1;
    return errno;
  }

  INFO("open '%s' succeeded.fd=%d", _device.c_str(), _fd);

  int version = 0x000800;
  unsigned char axes = 2;
  unsigned char buttons = 2;
#define NAME_LENGTH 128
  char name[NAME_LENGTH] = "Unknown";

  ioctl(_fd, JSIOCGVERSION, &version);
  ioctl(_fd, JSIOCGAXES, &axes);
  ioctl(_fd, JSIOCGBUTTONS, &buttons);
  ioctl(_fd, JSIOCGNAME(NAME_LENGTH), name);

  if (_axis == 0) _axis = (int *)calloc(_axes, sizeof(int));
  if (_button == 0) _button = (char *)calloc(_buttons, sizeof(char));

  INFO("Driver version is %d.%d.%d.", version >> 16, (version >> 8) & 0xff,
       version & 0xff);
  INFO("Axes : %d Buttons : %d ", axes, buttons);
  _connected = true;
  thread().addReadInvoker(_fd, this, onReadFd);
  thread().addErrorInvoker(_fd, this, [](void *pv) {
    Joystick *joystick = (Joystick *)pv;
    joystick->_connected = false;
    INFO("Disconnected from '%s' ", joystick->_device.c_str());
  });
  INFO("connected ");
  return 0;
}

int Joystick::disconnect() {
  if (!_connected) return 0;
  ::close(_fd);
  _fd = -1;
  _connected = false;
  return 0;
}

bool Joystick::exists() {
  struct stat buf;
  if (lstat(_device.c_str(), &buf) < 0) {
    return false;
  } else {
    return true;
  }
}

bool Joystick::connected() { return _connected; }

int Joystick::getEvent(EventClass &cls, int &instance, int &value) {
  int rc;
  struct js_event js;
  rc = read(_fd, &js, sizeof(struct js_event));
  if (rc == sizeof(struct js_event)) {
    instance = js.number;
    value = js.value;
    int kind = (js.type & ~JS_EVENT_INIT);
    if (kind == JS_EVENT_BUTTON)
      cls = EV_BUTTON;
    else if (kind == JS_EVENT_AXIS)
      cls = EV_AXIS;
    else
      cls = EV_INIT;
    return 0;
  }
  return ENODATA;
}
