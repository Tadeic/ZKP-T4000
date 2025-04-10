#!/bin/bash

# 复制驱动到系统目录
sudo cp eon.ko /lib/modules/$(uname -r)/kernel/drivers/gpu/
sudo depmod -a

# 配置开机加载
echo "eon" | sudo tee /etc/modules-load.d/eon.conf

# 加载驱动
sudo modprobe eon

# 签名（如果需要）
if [ -f eon.crt ] && [ -f eon.key ]; then
    sudo /usr/src/kernels/$(uname -r)/scripts/sign-file sha256 eon.key eon.crt eon.ko
fi

echo "安装完成！运行 'dmesg | grep eon' 查看日志。"
