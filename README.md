# Forest Fire Early Warning System using AWS

A real-time early warning system for forest fires using IoT sensors and AWS cloud services. This project combines hardware sensors with cloud processing to detect and alert for potential forest fire threats.

## 📋 Project Overview

This system uses:
- **ESP32/Arduino microcontrollers** equipped with temperature and humidity sensors
- **AWS IoT Core** for secure device communication
- **Real-time data processing** to detect anomalous fire conditions
- **Flutter mobile app** for alerts and monitoring
- **OLED displays** on devices for local status indication

## 🛠️ Hardware Components

- ESP32 Development Board
- DHT22/DHT11 Temperature & Humidity Sensor
- SSD1306 OLED Display (128x64)
- WiFi connectivity for AWS IoT communication

## 📁 Project Structure

```
├── aiec_01/                 # Primary IoT device code
│   └── aiec_01.ino          # Main sensor and AWS integration
├── aiec_02/                 # Secondary IoT device with OLED display
│   └── aiec_02.ino          # Enhanced version with OLED support
├── aiec_flutter/            # Mobile application
│   └── aiec_flutter.ino     # Flutter-based monitoring app
├── Report_*.docx            # Detailed project documentation
└── PPT_*.pptx               # Presentation materials
```

## 🚀 Getting Started

### Prerequisites

1. **Arduino IDE** - [Download here](https://www.arduino.cc/en/software)
2. **AWS Account** - [Create free tier account](https://aws.amazon.com/free/)
3. **Required Libraries**:
   - WiFi (built-in for ESP32)
   - WiFiClientSecure (built-in for ESP32)
   - PubSubClient (Install via Arduino Library Manager)
   - DHT sensor library by Adafruit
   - Adafruit GFX (for OLED support)
   - Adafruit SSD1306 (for OLED displays)

### Setup Instructions

#### 1. AWS IoT Setup
- Create an AWS IoT Thing in IoT Core
- Generate device certificates and download:
  - `cert.pem.crt` (Device Certificate)
  - `private.pem.key` (Private Key)
  - `AmazonRootCA1.pem` (CA Certificate)

#### 2. Configure Certificates
- Open `aiec_01.ino` or `aiec_02.ino` in Arduino IDE
- Locate the `AWS_CERT_CA[]`, `AWS_CERT_CRT[]`, and `AWS_CERT_KEY[]` sections
- Replace the placeholder content with your actual certificate data:
  ```cpp
  static const char AWS_CERT_CA[] PROGMEM = R"EOF(
  -----BEGIN CERTIFICATE-----
  [Your CA certificate content here]
  -----END CERTIFICATE-----
  )EOF";
  ```

#### 3. Update WiFi & AWS Credentials
In the `// -------- CONFIG ----------` section, update:
```cpp
#define WIFI_SSID        "Your_WiFi_SSID"
#define WIFI_PASSWORD    "Your_WiFi_Password"
#define AWS_IOT_ENDPOINT "your-endpoint.iot.region.amazonaws.com"
#define THINGNAME        "Your_Thing_Name"
```

#### 4. Upload to Device
- Connect ESP32 to your computer
- Select Board: ESP32 Dev Module
- Select appropriate COM port
- Click Upload

#### 5. Monitor Serial Output
- Open Serial Monitor (9600 baud)
- Verify WiFi connection and AWS IoT connection
- Monitor sensor readings

## 📊 Features

- ✅ Real-time temperature and humidity monitoring
- ✅ Secure AWS IoT Core connectivity
- ✅ Local OLED display for device status
- ✅ Flutter mobile application for remote monitoring
- ✅ WiFi-based remote connectivity
- ✅ Fire condition alerts and notifications

## 🔒 Security Notes

- **Never commit actual certificates** to version control
- Use AWS IoT certificate rotation regularly
- Keep WiFi credentials in environment variables or configuration files
- Use strong, unique passwords for AWS accounts

## 📱 Mobile App

The Flutter app (`aiec_flutter`) provides:
- Real-time sensor monitoring
- Fire risk alerts
- Historical data visualization
- Device management and configuration

## 📚 Documentation

- **Report**: See `Report_The Forest Fire Early Warning System using AWS.docx` for detailed technical documentation
- **Presentation**: See `PPT_The Forest Fire Early Warning System using AWS.pptx` for project overview

## 🤝 Contributing

Contributions are welcome! Please feel free to:
- Report bugs
- Suggest improvements
- Submit pull requests

## 📄 License

This project is provided as-is for educational and research purposes.



## 🙏 Acknowledgments

- AWS IoT documentation and samples
- Arduino and ESP32 communities
- Adafruit libraries and documentation
- Flutter development community

## 📞 Support

For issues, questions, or suggestions, please open an issue on GitHub.

---

**Last Updated**: 2026-06-15
