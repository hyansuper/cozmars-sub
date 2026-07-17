This part is a hook that executes before bootloader. It pulls low all pins connected to motors and servos. My intention is to prevent motors and servos jittering on power-up. But 

1. It maight not be necessary, these pins already default to low or high imp.
2. It doesn't fully serve the purpose -- during firmware flashing, one of the motors still runs.

ref: [custom-bootloader](https://docs.espressif.com/projects/esp-idf/en/v6.0.2/esp32c3/api-guides/bootloader.html#custom-bootloader)