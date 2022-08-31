# Smart Lights with WS2812 and ESP Rainmaker

A WS2812 Smart light written for ESP32 on Rainmaker for a complete DIY alternative solution to all well known market brands.

I've always loved how the [Nanoleaf Lights](https://nanoleaf.me/) looked as a decor item/accent lights in a room but it has always been a bit too expensive for me to ever get one.

And as electronics and software engineer, I figured it would be quite easy create my own version for like about the 10th of the cost and even more features (like motion detection)

## Demo
https://user-images.githubusercontent.com/24490575/187764227-740f462d-60de-43a5-915d-b5e42f166fa6.mp4

Note: The shadow parts are not as visbile with nakes eye as compared to the footage recorded on the camera (we can thank our eye's superiror dynamic range for that)

## Hardware Used
- ESP32
- WS2812 64 bit RGB Matrix
- RCWL-0516 Microwave Doppler motion detector
- 5V 5A adaptor (you may need to change this depending on the number of LEDs you plan to use)
- Wire connectors
- [3D printable hexagons (credits to DeDane1970)](https://www.thingiverse.com/thing:4831115/files)
- Good ol' 3M double sided tape

## Features
- WiFi connected System
- Fully controllable through ESP Rainmaker App
- Supports control with Google Assistant and Alexa
- Customize the lights to any color with color wheel
- Cycle gradually through all colour
- Split cycle mode giving each tile individual colours
- React to motion
- Turn on lights to low brightness when motion is detected

## Planned features
- Add OPT3001 to detect ambient light
- More options for colour transitions
- LED grouping Features
- Disco Mode

## Known Issues
- The setup can get stuck if brightness/saturation controls are used in Split cycle mode

