sudo cp joystick2redis.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl restart joystick2redis
sudo systemctl status joystick2redis
sudo systemctl enable joystick2redis
