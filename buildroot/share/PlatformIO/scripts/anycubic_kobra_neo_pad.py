#
# anycubic_kobra_neo_pad.py
#
# Replicates Anycubic's bin_expand.exe on Linux/macOS: pads firmware.bin with
# 0xFF bytes to a fixed total size of 0x77800 (489472) so the stock Kobra Neo
# bootloader will accept it from SD card.
#
# Bootloader region: 0x00000000-0x000087FF
# Application:       0x00008800-0x0007FFFF  (size 0x77800 on the 512K chip)
#
import os

Import("env")  # type: ignore  # noqa: F821 — provided by SCons/PlatformIO

TARGET_SIZE = 0x77800  # 489472 bytes
PAD_BYTE = b"\xff"


def pad_firmware(source, target, env):
    fw_path = str(target[0])
    if not fw_path.endswith(".bin"):
        return
    cur_size = os.path.getsize(fw_path)
    if cur_size > TARGET_SIZE:
        raise Exception(
            "Firmware too large for Kobra Neo flash region: "
            f"{cur_size} > {TARGET_SIZE} bytes"
        )
    pad_len = TARGET_SIZE - cur_size
    if pad_len == 0:
        return
    with open(fw_path, "ab") as f:
        f.write(PAD_BYTE * pad_len)
    print(
        f"Anycubic Kobra Neo: padded firmware.bin "
        f"{cur_size} -> {TARGET_SIZE} bytes (+{pad_len} 0xFF)"
    )


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", pad_firmware)  # type: ignore  # noqa: F821
