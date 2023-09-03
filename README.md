Esp32Cam Time Lapse

[Esp32Cam Time Lapse Server](https://github.com/starofys/esp32-time-lapse-server)

[Esp32Cam Web Config UI](https://github.com/starofys/esp32-time-lapse-web)


## 如果需要ota替换文件
if need ota replace file
> C:\Users\username\.platformio\packages\framework-arduinoespressif32\tools\partitions\huge_app.csv


###
```http request
### reboot
GET http://localhost:8080/reboot

### reload config
GET http://localhost:8080/reload

### current img
GET http://localhost:8080/capture

```

