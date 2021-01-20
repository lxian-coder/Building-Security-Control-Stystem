# Building-Security-Control-Stystem
This is an interesting C language Embedded System which i developted using arduino boards for a codeing assignment in university and I got full marks.

This is a building control system. This system got a friendly user interface. Users can easily control the operation of the system via Serial monitor. I will start from the interface to introduce different functions. When we begin to run the program, we can see the above image in Serial Monitor. This is the main menu of the system and when you press a letter or number, you can access different functions.

1. **When you press B** on the keyboard, you can modify the temperature range (Figure1). Enter the lowest value first and then the highest value. You need to be careful, because the number you entered must end with "*". The system will check the values you entered and If the low value is greater than the high value, they will not be accepted(Figure 3). If not, you will get a prompt statement 

1. **If you press C**, you can active new security cards. If the card be actived, people can use it to open the door. You can scan new cards one by one to active them and when the new card be actived successfully, the RGBLED will turn green and you can get a message on Serial monitor.But if the card has been already activited before, RGBLED will turn red and you will get a different message.

1. **If you press D**, you can delete security cards. If the card be deleted, it will not open the door.You can scan new cards one by one to delete them and if the card is not actived before, the RGBLED will turn red and you can get a pop up message. If not, RGBLED will turn red and you will get a different message. 

1. **If you press F**, you can delete Security card manually. In other words, you can delete cards by entering the card number. Because in reality, some people may lost their security cards. we must delete these lost cards but we can’t scan them.

1. **If you press E**, you can check the Security card list.

1. **If you press 0**, you can open safe mode. In safe mode, the door will be locked, RGBLED will blink and PIR sensor will start to work. If PIR detect anthying, you will get messege on the monitor. If you press 1, you can close safe mode. PIR Sensor will stop and everything back to normal

1. **If you press 2**, LEDarray will all on no matter what lightValue now is. In contrast, if you press 3, LEDarray will be all off. If you press 3, LEDarray back to auto and the lightValue will decide how many LEDarray turn on.

1. At any time you want to back to main menu, **press A**.
