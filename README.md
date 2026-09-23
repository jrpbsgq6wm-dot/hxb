# 项目归档仓库

本仓库集中存放各项目的源码、协议/技术文档和发行固件归档。各项目目录内的 README 记录了项目说明和角色映射。

| 角色 | 归档目录 | 内容 |
|---|---|---|
| svp | [SVP/](SVP/README.md) | SVP 源码、协议文档和发行固件 |
| 2040s | [2040s/](2040s/README.md) | 2040s 干端/湿端源码、协议文档和固件包 |
| 4070 | [4070/](4070/README.md) | 4070 源码、协议文档和发行固件包 |
| 23001 | [23001/](23001/README.md) | 23001 干端/湿端源码、文档和固件 |
| cubeame | [cubeame/](cubeame/README.md) | CUBEAM 项目源码、通信协议和固件包 |
| svs | [svs/](svs/README.md) | SVS 源码和数据传输协议 |

## 仓库信息

- Git 地址：git@github.com:jrpbsgq6wm-dot/hxb.git
- 默认分支：main
- @svp 映射到 SVP/；其余角色映射到同名目录。

源码归档已清理编译对象、依赖文件、构建输出、日志和 IDE 缓存；正式发行固件保留。

## 4070 固件恢复

4070 的两个发行 ZIP 以分卷形式保存。克隆仓库后，在仓库根目录运行以下 PowerShell 命令，即可重建 ZIP 并校验 SHA-256：

<code>powershell -File 4070/发行固件包/恢复固件包.ps1</code>
