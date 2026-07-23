set -euo pipefail

IMG="./build/ext2.hdd"
mkdir -p "$(dirname "$IMG")"

sudo rm -f -- "$IMG" || true
dd if=/dev/zero of="$IMG" bs=1M count=128

sgdisk -g "$IMG"
sgdisk "$IMG" -n 1:2048 -t 1:8300

LOOPDEV="$(sudo losetup -f --show --partscan "$IMG")"

# Wait a moment if /dev/loopXp1 appears slowly
sudo udevadm settle || true

sudo mkfs.ext2 -F -L DATA "${LOOPDEV}p1"

sudo losetup -d "$LOOPDEV"


