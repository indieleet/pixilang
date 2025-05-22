Low FPS on Raspberry Pi? Please follow the instructions below.

pixilang (with OpenGL):
  * in the raspi-config -> Advanced Options:
    * set GL driver to "full KMS";
    * enable Glamor;

pixilang_no_opengl (without OpenGL):
  * add the "softrender" option to the Pixilang config (~/.config/Pixilang/pixilang_config.ini);
  * in the raspi-config -> Advanced Options:
    * disable Glamor;
    * disable Compositor (xcompmgr);
