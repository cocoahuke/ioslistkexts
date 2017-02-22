# ioslistkexts
This tool will help list all the Kexts or extract the specified Kext from iOS kernel

[![Contact](https://img.shields.io/badge/contact-@cocoahuke-fbb52b.svg?style=flat)](https://twitter.com/cocoahuke) [![build](https://travis-ci.org/cocoahuke/ioslistkexts.svg?branch=master)](https://travis-ci.org/cocoahuke/ioskextdump) [![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/cocoahuke/iosdumpkernelfix/blob/master/LICENSE) [![paypal](https://img.shields.io/badge/Donate-PayPal-039ce0.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=EQDXSYW8Z23UY)

Supports ARM 32/64 bit iOS kernel. List file offset of kernel, size get from Mach-o header, interval size and BundleID for each Kexts

Tested in iOS8&iOS9 kernel cache, support 32/64 bit iOS kernel

###For kernel which dump from memory
Use [iosdumpkernelfix](https://github.com/cocoahuke/iosdumpkernelfix) to correct the Mach-O header before analyze it, Otherwise The analysis results are not complete list of Kexts

# How to use

**Download**
```bash
git clone https://github.com/cocoahuke/ioslistkexts.git \
&& cd ioslistkexts
```

**Compile and install** to /usr/local/bin/
```
make
make install
```

**Usage**
```
Usage: ioslistkexts <kernel file path> [-i <export item index> <export kext path>]
```

# Demo
I left a sample iOS8.3 kernelcache in the test directory

```
ioslistkexts test/iPhone6p_8.3_kernel.arm
```

Sample output:
```
...
147 items: kr_fileoff:0x11f7000  (Effective kext:140)
BundleID: com.apple.driver.AppleS5L8960XGPIOIC
macho file size:0x5000
interval size:0x5000

148 items: kr_fileoff:0x11fc000  (Effective kext:141)
BundleID: com.apple.driver.AppleT7000
macho file size:0x12000
interval size:0x14000

149 items: kr_fileoff:0x1210000  (Effective kext:142)
BundleID: com.apple.driver.AppleJPEGDriver
macho file size:0x15000
interval size:0x15000

150 items: kr_fileoff:0x1225000  (Effective kext:143)
BundleID: com.apple.driver.AppleSamsungI2S
macho file size:0x5000
interval size:0x5000
...
```

```
ioslistkexts test/iPhone6p_8.3_kernel.arm -i 149 test/AppleJPEGDriver.ke
```

Sample output:
```
...
149 items: kr_fileoff:0x1210000  (Effective kext:142)
BundleID: com.apple.driver.AppleJPEGDriver
macho file size:0x15000
---Export item 149---
export size:0x15000
saved successful in test/AppleJPEGDriver.ke
---Export end---
---prog end---
```
