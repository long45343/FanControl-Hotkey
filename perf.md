# FanControlHotkey 运行时性能报告 | Runtime Performance Report

> 采集时间 Collected: 2026-08-28 21:07 · 应用自启动于 Started: 2026-08-23 11:52:49
> 进程 Process: `FanControlHotkey.exe` (PID 17740) · `D:\EDCs\Tools\FancontrolHotkey\FanControlHotkey.exe`
> 主机 Host: AMD Ryzen 5 9600X (12 逻辑处理器 logical processors) · 31.7 GB RAM (空闲 free: 12.3 GB) · Windows 10.0.26100

## 核心指标 | Key Metrics

| 指标 Metric | 数值 Value | 评价 Assessment |
| --- | --- | --- |
| 连续运行时长 Uptime | 5 天 9 小时 14 分 (5d 9h 14m) | 长稳运行，未崩溃、未重启 Long-run stable, no crash, no restart |
| 窗口响应 Window responsiveness | Responding = True | GUI 消息循环健康 GUI message loop healthy |
| 累计 CPU 时间 Cumulative CPU | 952.06 s | 见下方归因 See attribution below |
| 生命周期平均 CPU Lifetime avg CPU | ≈ 0.20%（单核口径 single-core）/ ≈ 0.017%（整机折算 machine-normalized） | 极低 Negligible |
| 瞬时 CPU（5 秒采样 5s sample） | 0.62%（单核口径 single-core） | 正常抖动，与周期任务吻合 Normal jitter, consistent with the periodic task |
| 工作集 Working Set（当前 current / 峰值 peak） | 11.80 MB / 13.54 MB | 当前低于峰值，5 天无增长趋势 Below peak, no growth over 5 days |
| 私有提交内存 Private Commit | 2.63 MB | 极小，无内存泄漏迹象 Tiny, no sign of memory leak |
| 分页 / 非分页系统内存 Paged / Non-paged Pool | 0.16 MB / 0.01 MB | 忽略不计 Negligible |
| 虚拟地址空间 Virtual Address Space | 4212 MB | 64 位进程保留量，非实际占用，正常 64-bit reservation, not actual usage, normal |
| 线程数 Threads | 2 | 与 v1.1 设计一致：主 GUI 线程 + 监控线程 Matches v1.1 design: main GUI thread + monitor thread |
| 句柄数 Handles | 136 | 稳定偏低，无句柄泄漏 Stable and low, no handle leak |
| GDI / USER 对象 GDI / USER Objects | 13 / 12 | 远低于 10000 上限，无 GUI 资源泄漏 Far below the 10,000 limit, no GUI resource leak |
| 磁盘 I/O Disk I/O（读 read / 写 write） | ≈ 0.00 MB / 0.00 MB | 运行期不落盘，符合设计 No disk writes at runtime, as designed |
| 已加载模块 Loaded Modules | 28 | 纯原生依赖，规模正常 Pure native dependencies, normal scale |

## CPU 归因 | CPU Attribution

**中文**：累计 952.06 s CPU 中，951.77 s 集中在监控线程（TID 17760），主 GUI 线程（TID 17744）仅 0.27 s：

- 监控线程按 `src/app_context.h:15` 的 `POLL_INTERVAL_MS 2000` 每 2 秒经 `WaitForSingleObject` 超时唤醒（`src/process_monitor.c:119`），执行一次进程快照枚举与目标模式评估，每周期约 4 ms CPU。属"等待-唤醒"模型而非忙轮询，设计正确。
- 主 GUI 线程 5 天累计仅 0.27 s，说明热键消息循环几乎全空闲，热键按下可即时响应。
- 两个线程均处于 Wait/UserRequest 状态，优先级 Normal，符合预期。

**English**: Of the cumulative 952.06 s of CPU time, 951.77 s was spent in the monitor thread (TID 17760), while the main GUI thread (TID 17744) used only 0.27 s:

- The monitor thread wakes every 2 seconds via `WaitForSingleObject` timeout (`src/process_monitor.c:119`) as configured by `POLL_INTERVAL_MS 2000` in `src/app_context.h:15`, then performs one process-snapshot enumeration and target-mode evaluation — roughly 4 ms of CPU per cycle. This is a proper "wait-and-wake" model rather than busy polling.
- With only 0.27 s accumulated over 5 days, the GUI thread's hotkey message loop is essentially idle, so hotkey presses are answered instantly.
- Both threads sit in the Wait/UserRequest state at Normal priority, exactly as expected.

## 结论 | Conclusion

**中文**：应用连续运行 5 天以上，CPU 占用约 0.2%（单核口径），内存稳定在约 12 MB 且低于历史峰值，句柄、GDI/USER 对象、线程数均无泄漏迹象，磁盘 I/O 为零。各项指标与 v1.1 的异步监控线程架构完全吻合，长稳表现优秀，无需优化。

**English**: After more than 5 days of continuous operation, the app holds ~0.2% CPU (single-core view), memory is stable at ~12 MB and below its historical peak, and handles, GDI/USER objects, and thread count show no sign of leakage, with zero disk I/O. Every metric matches the v1.1 asynchronous monitor-thread architecture — excellent long-run stability, no optimization needed.
