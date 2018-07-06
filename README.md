# LPC43xx_M4_AnalogToDigital
Convert Analog(0-3.3V) Input(Ch3) to TTL(Active-High) Output(P1_3, GPIO0[10])

In the 5s period after board reset, threshold will be auto-adjusted according to the data range of the analog input(watch photodiode polarity, make sure input have maximum range), then digital output will be triggered using learned threshold. The digitized analog input will also be sent to analog output(Ch0).

![BoardConnect](./inc/Board Connection.JPG)