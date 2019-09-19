# LPC43xx_M4_AnalogToDigital
Convert Analog(0-3.3V) Input(Ch2; Ch3) to Digital(0/3.3V, Active-High) Output(P1_3, GPIO0[10]; P1_4, GPIO0[11]) on [LPCXpresso43xx Development Board](https://www.nxp.com/support/developer-resources/evaluation-and-development-boards/lpcxpresso-boards/lpcxpresso43s67-development-board:OM13084), also send Digitized Analog Input to Analog Output(Ch0; Ch1). Relay Digital Input(P1_5, GPIO1[8]) to Output(P1_6, GPIO1[9]).

In the 5s period after board power up or reset, threshold will be auto-adjusted according to the range of the analog input(check photodiode polarity, make sure input in maximum range, tested on [OSRAM BPW21](https://www.osram.com/os/ecat/Metal%20Can®%20TO39%20Ambient%20Light%20Sensor%20BPW%2021/com/en/class_pim_web_catalog_103489/global/prd_pim_device_2219533)), then digital output will be triggered using learned threshold.

![BoardConnect](./inc/BoardConnect.png)