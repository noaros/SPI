# SPI

After losing patience waiting for a "good" accelerometer to arrive, I ordered a cheap one, the HiLetGo BMI160. I found the pinout extremely confusing, but maybe understand it now. My interest was in the standard SPI pins, but the board used alternative names, and more than one pin looked like the same thing to me! There are differences between the chip pinout labels and the mapping to the actual breakout board. In addition the board supports I2C, and apparently has a secondary 3 wire bidirectional SPI interface to support output separately from the primary one. There are shared labels for all of that, with poor or hard to find documentation. One key was sleuthing that the BMI160 was very similar to the LSM6DS3 and hunted those docs instead. Hopefully the actual wiring will be the hardest part. I also don't have a reference tutorial for using this board, so time to leave training wheels behind.

<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/9d8dd31f-c06d-4c52-9682-24a86644f373" />

I switched to Arch for this project and after some difficulty managed to get a working STM32CubeProgrammer. I had hoped to use J-LinkCommander instead since it allows assembly debugging, but was disappointed to learn it only supports J-LINK. 

Not having a tutorial, I consulted AI of course. It provided code that I merged into mine, but it didn't work! Also it changed the pin assignments on me from one request to another, but I decided I liked the newer layout better, since used just one GPIO bank, and so rewired the board. When I switched to Gemini proper instead of just google search, I received instructions with checkpoints and verify steps mixed in. I liked this and started over again, piecing in the new code chunks to see if the tests passed.

I still had many issues however! Turns out I made a hilarious newbie mistake in electronics. I used double sided male header pins to connect the breakout board through its holes. This felt firm, so I thought I was good. But no, one needs to solder to actually get good electrical connections! Duh. 

Here is the new image, with redone connections, including different pin assignments (GPIO A4-A7).

<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/e0ca9f57-6cde-4b73-8a94-56c1e3aa71d8" />

That was progress, yet issues remained. I kept failing the sanity check of reading the chip id, getting 33 instead of 209. I tried many variations of AI generated code, with the plan of understanding and reworking it once I had a working starting point. Most failed, and I was even sent on some wild goose chases thinking I had a BMI270 instead! The issue seemed to be with the finicky requiring very special steps to trigger SPI mode instead of default I2C. I suspected the chip wasn't changing modes, and that resulted in some bits partially wrong on the returned number.

Finally one permutation of code changes, just trying to shift into SPI and sanity check, worked! What was the difference? That is hard to say as there are a number of subtle differences in the many code versions I tried. I tested a few, including a very interesting use of internal pull-up resistor not in any of the other versions, but to no avail. 

Next I decided to study, learn, and rework the working example.
