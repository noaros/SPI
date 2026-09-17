# SPI

After losing patience waiting for a "good" accelerometer to arrive, I ordered a cheap one, the HiLetGo BMI160. I found the pinout extremely confusing, but maybe understand it now. My interest was in the standard SPI pins, but the board used alternative names, and more than one pin looked like the same thing to me! There are differences between the chip pinout labels and the mapping to the actual breakout board. In addition the board supports I2C, and apparently has a secondary 3 wire bidirectional SPI interface to support output separately from the primary one. There are shared labels for all of that, with poor or hard to find documentation. One key was sleuthing that the BMI160 was very similar to the LSM6DS3 and hunted those docs instead. Hopefully the actual wiring will be the hardest part. I also don't have a reference tutorial for using this board, so time to leave training wheels behind.

<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/9d8dd31f-c06d-4c52-9682-24a86644f373" />

I switched to Arch for this project and after some difficulty managed to get a working STM32CubeProgrammer. I had hoped to use J-LinkCommander instead since it allows assembly debugging, but was disappointed to learn it only supports J-LINK. 

Not having a tutorial, I consulted AI of course. It provided code that didn't work! Also it changed the pin assignments on me from one request to another, but I decided I liked the newer layout better, since used just one GPIO bank, and so rewired the board.
