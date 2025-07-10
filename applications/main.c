//#include <rtthread.h>
//#include <rtdevice.h>
//#include <board.h>
//#include <serial.h>
//#include <rtdbg.h>
//
//#define PWM_DEV_NAME    "pwm2"
//#define PWM_CH1         1
//#define PWM_CH2         2
//#define DIR1_PIN        GET_PIN(G,0)
//#define DIR2_PIN        GET_PIN(G,1)
//
//#define STEPS_PER_REV   200    // 每圈步数，步距角 1.8°
//#define MICROSTEP       1     // 微步倍数
//#define TEST_REVOLUTIONS 8     // 测试转动圈数
//
///*=======================uart test=============================*/
//rt_device_t u2_dev;
//
//struct rt_semaphore sem;
//
//rt_thread_t u2_th;
//
//struct serial_configure u2_configs = RT_SERIAL_CONFIG_DEFAULT;
//
//rt_err_t rx_callback(rt_device_t dev, rt_size_t size)
//{
//    rt_sem_release(&sem);
//    return RT_EOK;
//}
//
//void serial_thread_entry(void *parameter)
//{
//    char buffer;
//    rt_size_t ret;
//
//    while(1)
//    {
//        ret = rt_device_read(u2_dev, 0, &buffer, 1);
//        if(ret == 1)
//        {
//            rt_sem_take(&sem, RT_WAITING_FOREVER);
//            rt_kprintf("%c", buffer);
//        }
//    }
//}
//
//
///*=======================uart test=============================*/
//
//
//static void stepper_thread(void *param)
//{
//    int channel = (int)param;
//    int dir_pin = (channel == PWM_CH1) ? DIR1_PIN : DIR2_PIN;
//    uint32_t revolutions = TEST_REVOLUTIONS;
//    uint32_t freq_hz = 30000;
//
//    struct rt_device_pwm *dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
//    if (!dev)
//    {
//        rt_kprintf("PWM device '%s' not found!\n", PWM_DEV_NAME);
//        return;
//    }
//
//    rt_pwm_enable(dev, channel);
//    rt_pin_mode(dir_pin, PIN_MODE_OUTPUT);
//    rt_pin_write(dir_pin, PIN_HIGH);  // 固定方向正转
//
//    uint32_t total_steps = STEPS_PER_REV * MICROSTEP * revolutions;   // 200 × 16 × 圈数
//    uint32_t period_ns = 1000000000UL / freq_hz;                      // 周期 ns
//    uint32_t pulse_period_us = period_ns / 1000UL;                    // 每个脉冲时间（us）
//    uint32_t total_time_us = total_steps * pulse_period_us;          // 总用时（us）
//
//    rt_pwm_set(dev, channel, period_ns, period_ns / 2);               // 50% 占空比
//
//    rt_kprintf("CH%d start: %lu rev (360°), freq %lu Hz, time=%lu us\n",
//               channel, revolutions, freq_hz, total_time_us);
//
//    rt_thread_delay(rt_tick_from_millisecond(total_time_us / 1000)); // 毫秒延时
//
//    rt_pwm_disable(dev, channel);
//    rt_kprintf("CH%d stop\n", channel);
//}
//
//int main(void)
//{
//    /*============uart test===============*/
//    rt_err_t ret = 0;
//    u2_dev = rt_device_find("uart2");
//    if(u2_dev == RT_NULL)
//    {
//        rt_kprintf("rt_device_find[uart2] failed...\n");
//        return RT_EINVAL;
//    }
//    ret = rt_device_open(u2_dev,RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
//    if(ret < 0)
//    {
//        rt_kprintf("rt_device_open[uart2] failed...\n");
//        return ret;
//    }
//
//    rt_device_control(u2_dev, RT_DEVICE_CTRL_CONFIG, (void*)&u2_configs);
//
//    rt_device_set_rx_indicate(u2_dev, rx_callback);
//
//    rt_sem_init(&sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
//
//    u2_th = rt_thread_create("u2_recv", serial_thread_entry, NULL, 1024, 10, 5);
//
//    rt_thread_startup(u2_th);
//
//    rt_device_write(u2_dev, 0, "UART2 config...\n", rt_strlen("UART2 config...\n"));
//    /*============uart test===============*/
//    rt_thread_t t1, t2;
//
//    rt_thread_mdelay(800);  // 开机延迟
//
//    t1 = rt_thread_create("step1", stepper_thread, (void *)PWM_CH1,
//                          512, RT_THREAD_PRIORITY_MAX - 2, 10);
//    t2 = rt_thread_create("step2", stepper_thread, (void *)PWM_CH2,
//                          512, RT_THREAD_PRIORITY_MAX - 2, 10);
//
//    if (t1) rt_thread_startup(t1);
//    if (t2) rt_thread_startup(t2);
//
//    return RT_EOK;
//}

//=====================uart测试代码=======================//
// 串口通信示例，严格限定：起始位是x，结束位是y，忽略无效数据，自动同步起始x

//#include <rtthread.h>
//#include <rtdevice.h>
//#include <board.h>
//#include <serial.h>
//#include <rtdbg.h>
//#include <string.h>
//
//#define UART_RX_NAME   "uart2"
//#define UART_TX_NAME   "uart1"
//#define DATA_LEN       8
//
//static rt_device_t uart_rx_dev = RT_NULL;
//static rt_device_t uart_tx_dev = RT_NULL;
//
//static struct rt_semaphore sem_rx, sem_tx;
//
//// 校验函数：第1位必须是x，第8位（最后一位）必须是y
//static rt_bool_t check_xy_format(const char* buf)
//{
//    if (!buf) return RT_FALSE;
//    if (buf[0] != 'x') return RT_FALSE;
//    if (buf[7] != 'y') return RT_FALSE;
//    return RT_TRUE;
//}
//
//// 辅助函数：同步到下一个x，丢弃无效数据
//static void sync_to_x(rt_device_t dev, struct rt_semaphore* sem)
//{
//    char ch;
//    while (1)
//    {
//        int ret = rt_device_read(dev, 0, &ch, 1);
//        if (ret == 1)
//        {
//            if (ch == 'x') {
//                // 把x放回缓冲区开头
//                rt_device_read(dev, 0, NULL, 0); // 清除可能的其他数据（可选）
//                // 将x作为下一个包的第一个字符
//                // 直接返回，主线程会把它当作第一个字节存入数组
//                break;
//            }
//            // 否则继续丢弃
//        }
//        else
//        {
//            rt_sem_take(sem, RT_WAITING_FOREVER);
//        }
//    }
//}
//
///* UART2 线程：接收外设数据，严格首x尾y，不符合则忽略并同步到下一个x */
//static void uart2_thread(void *param)
//{
//    char buf[DATA_LEN];
//    char xy[DATA_LEN + 1]; // 8字节+结尾符
//    while (1)
//    {
//        // 寻找第一个x
//        sync_to_x(uart_rx_dev, &sem_rx);
//        buf[0] = 'x';
//        int read_size = 1;
//        // 读取余下字节
//        while (read_size < DATA_LEN)
//        {
//            int ret = rt_device_read(uart_rx_dev, 0, buf + read_size, DATA_LEN - read_size);
//            if (ret > 0)
//                read_size += ret;
//            else
//                rt_sem_take(&sem_rx, RT_WAITING_FOREVER);
//        }
//        // 校验：首位x，第8位y
//        if (check_xy_format(buf))
//        {
//            memcpy(xy, buf, DATA_LEN);
//            xy[DATA_LEN] = '\0';
//            rt_kprintf("UART2 Recv Valid: %s\n", xy);
//            // 可进一步处理 xy
//        }
//        // 不符合不做任何提示，直接忽略并同步到下一个x
//    }
//}
//
///* UART1 线程：同理实现（如需也校验可仿照处理） */
//static void uart1_thread(void *param)
//{
//    char buf[DATA_LEN];
//    char data[DATA_LEN + 1];
//    while (1)
//    {
//        // 寻找第一个x
//        sync_to_x(uart_tx_dev, &sem_tx);
//        buf[0] = 'x';
//        int read_size = 1;
//        while (read_size < DATA_LEN)
//        {
//            int ret = rt_device_read(uart_tx_dev, 0, buf + read_size, DATA_LEN - read_size);
//            if (ret > 0)
//                read_size += ret;
//            else
//                rt_sem_take(&sem_tx, RT_WAITING_FOREVER);
//        }
//        if (check_xy_format(buf))
//        {
//            memcpy(data, buf, DATA_LEN);
//            data[DATA_LEN] = '\0';
//            rt_kprintf("UART1 Recv Valid: %s\n", data);
//        }
//        // 不符合不打印，继续同步下一个x
//    }
//}
//
//static rt_err_t rx2_callback(rt_device_t dev, rt_size_t size)
//{
//    rt_sem_release(&sem_rx);
//    return RT_EOK;
//}
//static rt_err_t rx1_callback(rt_device_t dev, rt_size_t size)
//{
//    rt_sem_release(&sem_tx);
//    return RT_EOK;
//}
//
//int main(void)
//{
//    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
//
//    uart_rx_dev = rt_device_find(UART_RX_NAME);
//    RT_ASSERT(uart_rx_dev);
//    rt_device_open(uart_rx_dev, RT_DEVICE_FLAG_INT_RX);
//    rt_device_control(uart_rx_dev, RT_DEVICE_CTRL_CONFIG, &cfg);
//    rt_sem_init(&sem_rx, "sem_rx", 0, RT_IPC_FLAG_FIFO);
//    rt_device_set_rx_indicate(uart_rx_dev, rx2_callback);
//    rt_thread_t t2 = rt_thread_create("uart2", uart2_thread, RT_NULL, 1024, 20, 10);
//    rt_thread_startup(t2);
//
//    uart_tx_dev = rt_device_find(UART_TX_NAME);
//    RT_ASSERT(uart_tx_dev);
//    rt_device_open(uart_tx_dev, RT_DEVICE_FLAG_INT_RX);
//    rt_device_control(uart_tx_dev, RT_DEVICE_CTRL_CONFIG, &cfg);
//    rt_sem_init(&sem_tx, "sem_tx", 0, RT_IPC_FLAG_FIFO);
//    rt_device_set_rx_indicate(uart_tx_dev, rx1_callback);
//    rt_thread_t t1 = rt_thread_create("uart1", uart1_thread, RT_NULL, 1024, 20, 10);
//    rt_thread_startup(t1);
//
//    rt_device_write(uart_rx_dev, 0, "UART2 ready (外设→MCU)\r\n", strlen("UART2 ready (外设→MCU)\r\n"));
//    rt_device_write(uart_tx_dev, 0, "UART1 ready (MCU→PC)\r\n", strlen("UART1 ready (MCU→PC)\r\n"));
//
//    return RT_EOK;
//}
//==========================================================//
// 串口通信示例，严格限定：起始位是x，结束位是y，忽略无效数据，自动同步起始x


//=====================================================//

//#include <rtthread.h>
//#include <rtdevice.h>
//#include <board.h>
//#include <serial.h>
//#include <rtdbg.h>
//
//#define PWM_DEV_NAME    "pwm2"
//#define PWM_CH1         1
//#define PWM_CH2         2
//#define DIR1_PIN        GET_PIN(G,0)
//#define DIR2_PIN        GET_PIN(G,1)
//
//#define STEPS_PER_REV   200    // 每圈步数，步距角 1.8°
//#define MICROSTEP       16     // 微步倍数
//
//// 分别定义两个channel的转动圈数
//#define CH1_REVOLUTIONS 25      // Channel1 转动圈数
//#define CH2_REVOLUTIONS 15      // Channel2 转动圈数
//
//// 方向控制宏定义 - 在这里调整方向
//#define CH1_DIRECTION   PIN_HIGH    // Channel1方向：PIN_HIGH(正方向) 或 PIN_LOW(反方向)
//#define CH2_DIRECTION   PIN_HIGH     // Channel2方向：PIN_HIGH(正方向) 或 PIN_LOW(反方向)
//
///*=======================uart test=============================*/
//rt_device_t u2_dev;    // 原 uart2 用于 PC→MCU
//rt_device_t u1_dev;    // 新增 uart1 用于 MCU→PC
//struct rt_semaphore sem2, sem1;
//
//// 添加同步信号量，确保channel1先执行
//static struct rt_semaphore ch1_complete_sem;
//
//struct serial_configure u2_configs = RT_SERIAL_CONFIG_DEFAULT;
//
///* 中断回调 */
//rt_err_t rx2_callback(rt_device_t dev, rt_size_t size)
//{
//    rt_sem_release(&sem2);
//    return RT_EOK;
//}
//rt_err_t rx1_callback(rt_device_t dev, rt_size_t size)
//{
//    rt_sem_release(&sem1);
//    return RT_EOK;
//}
//
///* UART2 收 PC 发给 MCU 的线程 */
//void serial2_thread_entry(void *parameter)
//{
//    char buffer;
//    while(1)
//    {
//        while (rt_device_read(u2_dev, 0, &buffer, 1) != 1)
//            rt_sem_take(&sem2, RT_WAITING_FOREVER);
//        rt_kprintf("UART2 RX→MCU: %c\n", buffer);
//        /* 回复 MCU 到 PC：通过 UART1 发送 */
//        rt_device_write(u1_dev, 0, &buffer, 1);
//    }
//}
//
///* UART1 收 MCU 数据（可用于调试或桥接） */
//void serial1_thread_entry(void *parameter)
//{
//    char buffer;
//    while(1)
//    {
//        while (rt_device_read(u1_dev, 0, &buffer, 1) != 1)
//            rt_sem_take(&sem1, RT_WAITING_FOREVER);
//        rt_kprintf("UART1 RX←MCU: %c\n", buffer);
//        /* 如果需要，再透传到 PC 上的 UART2 */
//        rt_device_write(u2_dev, 0, &buffer, 1);
//    }
//}
//
///*=======================uart test end=============================*/
//
//// 改进的步进电机控制函数 - 使用宏定义控制方向
//static void stepper_control(int channel, uint32_t revolutions, uint32_t freq_hz)
//{
//    int dir_pin = (channel == PWM_CH1) ? DIR1_PIN : DIR2_PIN;
//
//    struct rt_device_pwm *dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
//    if (!dev)
//    {
//        rt_kprintf("PWM device '%s' not found!\n", PWM_DEV_NAME);
//        return;
//    }
//
//    rt_pwm_enable(dev, channel);
//    rt_pin_mode(dir_pin, PIN_MODE_OUTPUT);
//
//    // 使用宏定义设置方向
//    if (channel == PWM_CH1) {
//        rt_pin_write(dir_pin, CH1_DIRECTION);
//        rt_kprintf("CH1 direction: %s (%s)\n",
//                   (CH1_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD",
//                   (CH1_DIRECTION == PIN_HIGH) ? "HIGH" : "LOW");
//    } else if (channel == PWM_CH2) {
//        rt_pin_write(dir_pin, CH2_DIRECTION);
//        rt_kprintf("CH2 direction: %s (%s)\n",
//                   (CH2_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD",
//                   (CH2_DIRECTION == PIN_HIGH) ? "HIGH" : "LOW");
//    }
//
//    uint32_t total_steps = (STEPS_PER_REV * MICROSTEP * revolutions) - 50;
//    uint32_t period_ns = 1000000000UL / freq_hz;
//    uint32_t pulse_period_us = period_ns / 1000UL;
//    uint32_t total_time_us = total_steps * pulse_period_us;
//
//    rt_pwm_set(dev, channel, period_ns, period_ns / 2);
//
//    rt_kprintf("CH%d start: %lu rev, freq %lu Hz, time=%lu us\n",
//               channel, revolutions, freq_hz, total_time_us);
//
//    // 发送开始信息到PC
//    char start_msg[150];
//    const char* dir_str = "";
//    if (channel == PWM_CH1) {
//        dir_str = (CH1_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD";
//    } else {
//        dir_str = (CH2_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD";
//    }
//
//    rt_snprintf(start_msg, sizeof(start_msg),
//                "CH%d START: %lu revolutions, %s direction\r\n",
//                channel, revolutions, dir_str);
//    rt_device_write(u1_dev, 0, start_msg, rt_strlen(start_msg));
//
//    // 执行转动
//    rt_thread_delay(rt_tick_from_millisecond(total_time_us / 1000));
//
//    rt_pwm_disable(dev, channel);
//    rt_kprintf("CH%d stop\n", channel);
//
//    // 发送完成信息到PC
//    char complete_msg[150];
//    rt_snprintf(complete_msg, sizeof(complete_msg),
//                "CH%d COMPLETE: %lu revolutions finished, %s direction\r\n",
//                channel, revolutions, dir_str);
//    rt_device_write(u1_dev, 0, complete_msg, rt_strlen(complete_msg));
//}
//
//// Channel1 线程 - 先执行
//static void stepper_ch1_thread(void *param)
//{
//    uint32_t freq_hz = 30000;
//
//    rt_kprintf("=== Channel1 Thread Started ===\n");
//
//    // 执行Channel1的转动
//    stepper_control(PWM_CH1, CH1_REVOLUTIONS, freq_hz);
//
//    // 发送完成信号，允许Channel2开始
//    rt_sem_release(&ch1_complete_sem);
//
//    rt_kprintf("=== Channel1 Thread Finished ===\n");
//}
//
//// Channel2 线程 - 等待Channel1完成后执行
//static void stepper_ch2_thread(void *param)
//{
//    uint32_t freq_hz = 30000;
//
//    rt_kprintf("=== Channel2 Thread Started, waiting for CH1 ===\n");
//
//    // 等待Channel1完成
//    rt_sem_take(&ch1_complete_sem, RT_WAITING_FOREVER);
//
//    rt_kprintf("=== Channel1 completed, starting Channel2 ===\n");
//
//    // 发送等待完成信息到PC
//    char wait_msg[] = "CH1 completed, CH2 starting...\r\n";
//    rt_device_write(u1_dev, 0, wait_msg, rt_strlen(wait_msg));
//
//    // 执行Channel2的转动
//    stepper_control(PWM_CH2, CH2_REVOLUTIONS, freq_hz);
//
//    rt_kprintf("=== Channel2 Thread Finished ===\n");
//
//    // 发送整个流程完成信息到PC
//    char all_complete_msg[] = "All motor movements completed!\r\n";
//    rt_device_write(u1_dev, 0, all_complete_msg, rt_strlen(all_complete_msg));
//}
//
//int main(void)
//{
//    /*============uart test===============*/
//    rt_err_t ret = RT_EOK;
//
//    /* 初始化 UART2（PC→MCU）*/
//    u2_dev = rt_device_find("uart2");
//    if (u2_dev == RT_NULL)
//    {
//        LOG_E("rt_device_find[uart2] failed...\n");
//        return RT_EINVAL;
//    }
//    ret = rt_device_open(u2_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
//    if (ret < 0)
//    {
//        LOG_E("rt_device_open[uart2] failed...\n");
//        return ret;
//    }
//    rt_device_control(u2_dev, RT_DEVICE_CTRL_CONFIG, (void*)&u2_configs);
//    rt_sem_init(&sem2, "rx2", 0, RT_IPC_FLAG_FIFO);
//    rt_device_set_rx_indicate(u2_dev, rx2_callback);
//    rt_thread_t u2_th = rt_thread_create("uart2_r", serial2_thread_entry, RT_NULL, 1024, 10, 5);
//    rt_thread_startup(u2_th);
//
//    /* 初始化 UART1（MCU→PC）*/
//    u1_dev = rt_device_find("uart1");
//    if (u1_dev == RT_NULL)
//    {
//        LOG_E("rt_device_find[uart1] failed...\n");
//        return RT_EINVAL;
//    }
//    ret = rt_device_open(u1_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
//    if (ret < 0)
//    {
//        LOG_E("rt_device_open[uart1] failed...\n");
//        return ret;
//    }
//    rt_device_control(u1_dev, RT_DEVICE_CTRL_CONFIG, (void*)&u2_configs);
//    rt_sem_init(&sem1, "rx1", 0, RT_IPC_FLAG_FIFO);
//    rt_device_set_rx_indicate(u1_dev, rx1_callback);
//    rt_thread_t u1_th = rt_thread_create("uart1_r", serial1_thread_entry, RT_NULL, 1024, 10, 5);
//    rt_thread_startup(u1_th);
//
//    /* 初始消息发送 */
//    const char *msg2 = "UART2 config...\n";
//    rt_device_write(u2_dev, 0, msg2, rt_strlen(msg2));
//    const char *msg1 = "UART1 ready...\n";
//    rt_device_write(u1_dev, 0, msg1, rt_strlen(msg1));
//    /*============uart test===============*/
//
//    // 初始化同步信号量
//    rt_sem_init(&ch1_complete_sem, "ch1_done", 0, RT_IPC_FLAG_FIFO);
//
//    rt_thread_mdelay(1000);  // 开机延时
//
//    // 发送系统配置信息
//    char config_msg[200];
//    rt_snprintf(config_msg, sizeof(config_msg),
//                "System Config: CH1=%d revs(%s), CH2=%d revs(%s), CH1 first\r\n",
//                CH1_REVOLUTIONS, (CH1_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD",
//                CH2_REVOLUTIONS, (CH2_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD");
//    rt_device_write(u1_dev, 0, config_msg, rt_strlen(config_msg));
//
//    rt_kprintf("Starting sequential stepper control:\n");
//    rt_kprintf("- Channel1: %d revolutions, %s direction (first)\n",
//               CH1_REVOLUTIONS, (CH1_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD");
//    rt_kprintf("- Channel2: %d revolutions, %s direction (after CH1)\n",
//               CH2_REVOLUTIONS, (CH2_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD");
//
//    // 显示方向控制信息
//    rt_kprintf("Direction Control:\n");
//    rt_kprintf("- CH1_DIRECTION = %s\n", (CH1_DIRECTION == PIN_HIGH) ? "PIN_HIGH" : "PIN_LOW");
//    rt_kprintf("- CH2_DIRECTION = %s\n", (CH2_DIRECTION == PIN_HIGH) ? "PIN_HIGH" : "PIN_LOW");
//
//    /* 创建按顺序执行的步进线程 */
//    rt_thread_t t1 = rt_thread_create("step_ch1", stepper_ch1_thread, RT_NULL,
//                                      1024, RT_THREAD_PRIORITY_MAX - 2, 10);
//    rt_thread_t t2 = rt_thread_create("step_ch2", stepper_ch2_thread, RT_NULL,
//                                      1024, RT_THREAD_PRIORITY_MAX - 2, 10);
//
//    if (t1) {
//        rt_thread_startup(t1);
//        rt_kprintf("Channel1 thread started\n");
//    }
//    if (t2) {
//        rt_thread_startup(t2);
//        rt_kprintf("Channel2 thread started (waiting for CH1)\n");
//    }
//
//    return RT_EOK;
//}
//#include <rtthread.h>
//#include <rtdevice.h>
//#include <board.h>
//#include <serial.h>
//#include <rtdbg.h>
//
//#define PWM_DEV_NAME    "pwm2"
//#define PWM_CH1         1
//#define PWM_CH2         2
//#define DIR1_PIN        GET_PIN(G,0)
//#define DIR2_PIN        GET_PIN(G,1)
//
//#define STEPS_PER_REV   200    // 每圈步数，步距角 1.8°
//#define MICROSTEP       16     // 微步倍数
//
//// 第一次运转的圈数设定
//#define CH1_RUN1_REVOLUTIONS 25      // Channel1 第一次转动圈数
//#define CH2_RUN1_REVOLUTIONS 15      // Channel2 第一次转动圈数
//
//// 第二次运转的圈数设定
//#define CH1_RUN2_REVOLUTIONS 10      // Channel1 第二次转动圈数
//#define CH2_RUN2_REVOLUTIONS 8       // Channel2 第二次转动圈数
//
//// 反转回退的圈数设定
//#define CH1_RETURN_REVOLUTIONS 15    // Channel1 反转回退圈数
//#define CH2_RETURN_REVOLUTIONS 23    // Channel2 反转回退圈数
//
//// 运转间隔时间
//#define RUN_INTERVAL_MS 10000         // 两次运转间隔时间（5秒）
//#define RETURN_WAIT_MS  5000         // 反转前等待时间（5秒）
//
//// 第一次运转方向控制宏定义
//#define CH1_RUN1_DIRECTION   PIN_HIGH    // Channel1 第一次运转方向
//#define CH2_RUN1_DIRECTION   PIN_HIGH    // Channel2 第一次运转方向
//
//// 第二次运转方向控制宏定义
//#define CH1_RUN2_DIRECTION   PIN_LOW     // Channel1 第二次运转方向
//#define CH2_RUN2_DIRECTION   PIN_HIGH    // Channel2 第二次运转方向
//
//// 反转回退方向控制宏定义（通常与第二次运转方向相反）
//#define CH1_RETURN_DIRECTION (!CH2_RUN2_DIRECTION)  // Channel1 反转方向
//#define CH2_RETURN_DIRECTION (!CH2_RUN2_DIRECTION)  // Channel2 反转方向
//
///*=======================uart test=============================*/
//rt_device_t u2_dev;    // 原 uart2 用于 PC→MCU
//rt_device_t u1_dev;    // 新增 uart1 用于 MCU→PC
//struct rt_semaphore sem2, sem1;
//
//// 添加同步信号量，控制运转顺序
//static struct rt_semaphore ch1_run1_complete_sem;
//static struct rt_semaphore ch2_run1_complete_sem;
//static struct rt_semaphore ch1_run2_complete_sem;
//static struct rt_semaphore ch2_run2_complete_sem;
//static struct rt_semaphore ch1_return_complete_sem;
//static struct rt_semaphore ch2_return_complete_sem;
//
//struct serial_configure u2_configs = RT_SERIAL_CONFIG_DEFAULT;
//
///* 中断回调 */
//rt_err_t rx2_callback(rt_device_t dev, rt_size_t size)
//{
//    rt_sem_release(&sem2);
//    return RT_EOK;
//}
//rt_err_t rx1_callback(rt_device_t dev, rt_size_t size)
//{
//    rt_sem_release(&sem1);
//    return RT_EOK;
//}
//
///* UART2 收 PC 发给 MCU 的线程 */
//void serial2_thread_entry(void *parameter)
//{
//    char buffer;
//    while(1)
//    {
//        while (rt_device_read(u2_dev, 0, &buffer, 1) != 1)
//            rt_sem_take(&sem2, RT_WAITING_FOREVER);
//        rt_kprintf("UART2 RX→MCU: %c\n", buffer);
//        /* 回复 MCU 到 PC：通过 UART1 发送 */
//        rt_device_write(u1_dev, 0, &buffer, 1);
//    }
//}
//
///* UART1 收 MCU 数据（可用于调试或桥接） */
//void serial1_thread_entry(void *parameter)
//{
//    char buffer;
//    while(1)
//    {
//        while (rt_device_read(u1_dev, 0, &buffer, 1) != 1)
//            rt_sem_take(&sem1, RT_WAITING_FOREVER);
//        rt_kprintf("UART1 RX←MCU: %c\n", buffer);
//        /* 如果需要，再透传到 PC 上的 UART2 */
//        rt_device_write(u2_dev, 0, &buffer, 1);
//    }
//}
//
///*=======================uart test end=============================*/
//
//// 改进的步进电机控制函数 - 分别控制三次运转的方向
//static void stepper_control(int channel, uint32_t revolutions, uint32_t freq_hz, int run_number)
//{
//    int dir_pin = (channel == PWM_CH1) ? DIR1_PIN : DIR2_PIN;
//    rt_base_t direction;
//
//    // 根据channel和run_number确定方向
//    if (channel == PWM_CH1) {
//        if (run_number == 1) {
//            direction = CH1_RUN1_DIRECTION;
//        } else if (run_number == 2) {
//            direction = CH1_RUN2_DIRECTION;
//        } else { // run_number == 3 (return)
//            direction = CH1_RETURN_DIRECTION;
//        }
//    } else {
//        if (run_number == 1) {
//            direction = CH2_RUN1_DIRECTION;
//        } else if (run_number == 2) {
//            direction = CH2_RUN2_DIRECTION;
//        } else { // run_number == 3 (return)
//            direction = CH2_RETURN_DIRECTION;
//        }
//    }
//
//    struct rt_device_pwm *dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
//    if (!dev)
//    {
//        rt_kprintf("PWM device '%s' not found!\n", PWM_DEV_NAME);
//        return;
//    }
//
//    rt_pwm_enable(dev, channel);
//    rt_pin_mode(dir_pin, PIN_MODE_OUTPUT);
//
//    // 设置方向
//    rt_pin_write(dir_pin, direction);
//
//    const char* run_type = (run_number == 3) ? "RETURN" : "RUN";
//    rt_kprintf("CH%d %s%d direction: %s (%s)\n",
//               channel, run_type, (run_number == 3) ? 0 : run_number,
//               (direction == PIN_HIGH) ? "FORWARD" : "BACKWARD",
//               (direction == PIN_HIGH) ? "HIGH" : "LOW");
//
//    uint32_t total_steps = (STEPS_PER_REV * MICROSTEP * revolutions) - 50;
//    uint32_t period_ns = 1000000000UL / freq_hz;
//    uint32_t pulse_period_us = period_ns / 1000UL;
//    uint32_t total_time_us = total_steps * pulse_period_us;
//
//    rt_pwm_set(dev, channel, period_ns, period_ns / 2);
//
//    rt_kprintf("CH%d %s%d start: %lu rev, freq %lu Hz, time=%lu us\n",
//               channel, run_type, (run_number == 3) ? 0 : run_number,
//               revolutions, freq_hz, total_time_us);
//
//    // 发送开始信息到PC
//    char start_msg[200];
//    const char* dir_str = (direction == PIN_HIGH) ? "FORWARD" : "BACKWARD";
//
//    rt_snprintf(start_msg, sizeof(start_msg),
//                "CH%d %s%d START: %lu revolutions, %s direction\r\n",
//                channel, run_type, (run_number == 3) ? 0 : run_number,
//                revolutions, dir_str);
//    rt_device_write(u1_dev, 0, start_msg, rt_strlen(start_msg));
//
//    // 执行转动
//    rt_thread_delay(rt_tick_from_millisecond(total_time_us / 1000));
//
//    rt_pwm_disable(dev, channel);
//    rt_kprintf("CH%d %s%d stop\n", channel, run_type, (run_number == 3) ? 0 : run_number);
//
//    // 发送完成信息到PC
//    char complete_msg[200];
//    rt_snprintf(complete_msg, sizeof(complete_msg),
//                "CH%d %s%d COMPLETE: %lu revolutions finished, %s direction\r\n",
//                channel, run_type, (run_number == 3) ? 0 : run_number,
//                revolutions, dir_str);
//    rt_device_write(u1_dev, 0, complete_msg, rt_strlen(complete_msg));
//}
//
//// Channel1 线程 - 执行两次运转 + 反转回退
//static void stepper_ch1_thread(void *param)
//{
//    uint32_t freq_hz = 30000;
//
//    rt_kprintf("=== Channel1 Thread Started ===\n");
//
//    // 第一次运转
//    rt_kprintf("=== CH1 First Run ===\n");
//    stepper_control(PWM_CH1, CH1_RUN1_REVOLUTIONS, freq_hz, 1);
//
//    // 发送第一次完成信号
//    rt_sem_release(&ch1_run1_complete_sem);
//    rt_kprintf("=== CH1 First Run Finished ===\n");
//
//    // 等待CH2第一次运转完成
//    rt_sem_take(&ch2_run1_complete_sem, RT_WAITING_FOREVER);
//
//    // 间隔延时
//    rt_kprintf("=== Waiting %d seconds before second run ===\n", RUN_INTERVAL_MS / 1000);
//    rt_device_write(u1_dev, 0, "Waiting 5 seconds before second run...\r\n", 40);
//    rt_thread_delay(rt_tick_from_millisecond(RUN_INTERVAL_MS));
//
//    // 第二次运转
//    rt_kprintf("=== CH1 Second Run ===\n");
//    stepper_control(PWM_CH1, CH1_RUN2_REVOLUTIONS, freq_hz, 2);
//
//    // 发送第二次完成信号
//    rt_sem_release(&ch1_run2_complete_sem);
//    rt_kprintf("=== CH1 Second Run Finished ===\n");
//
//    // 等待CH2第二次运转完成
//    rt_sem_take(&ch2_run2_complete_sem, RT_WAITING_FOREVER);
//
//    // 反转前等待
//    rt_kprintf("=== Waiting %d seconds before return movement ===\n", RETURN_WAIT_MS / 1000);
//    rt_device_write(u1_dev, 0, "Waiting 5 seconds before return movement...\r\n", 48);
//    rt_thread_delay(rt_tick_from_millisecond(RETURN_WAIT_MS));
//
//    // 反转回退
//    rt_kprintf("=== CH1 Return Movement ===\n");
//    stepper_control(PWM_CH1, CH1_RETURN_REVOLUTIONS, freq_hz, 3);
//
//    // 发送反转完成信号
//    rt_sem_release(&ch1_return_complete_sem);
//    rt_kprintf("=== CH1 Return Movement Finished ===\n");
//
//    rt_kprintf("=== Channel1 Thread Finished ===\n");
//}
//
//// Channel2 线程 - 执行两次运转 + 反转回退
//static void stepper_ch2_thread(void *param)
//{
//    uint32_t freq_hz = 30000;
//
//    rt_kprintf("=== Channel2 Thread Started, waiting for CH1 ===\n");
//
//    // 等待CH1第一次运转完成
//    rt_sem_take(&ch1_run1_complete_sem, RT_WAITING_FOREVER);
//
//    // 第一次运转
//    rt_kprintf("=== CH2 First Run ===\n");
//    rt_device_write(u1_dev, 0, "CH1 run1 completed, CH2 run1 starting...\r\n", 42);
//    stepper_control(PWM_CH2, CH2_RUN1_REVOLUTIONS, freq_hz, 1);
//
//    // 发送第一次完成信号
//    rt_sem_release(&ch2_run1_complete_sem);
//    rt_kprintf("=== CH2 First Run Finished ===\n");
//
//    // 等待CH1第二次运转完成
//    rt_sem_take(&ch1_run2_complete_sem, RT_WAITING_FOREVER);
//
//    // 第二次运转
//    rt_kprintf("=== CH2 Second Run ===\n");
//    rt_device_write(u1_dev, 0, "CH1 run2 completed, CH2 run2 starting...\r\n", 42);
//    stepper_control(PWM_CH2, CH2_RUN2_REVOLUTIONS, freq_hz, 2);
//
//    // 发送第二次完成信号
//    rt_sem_release(&ch2_run2_complete_sem);
//    rt_kprintf("=== CH2 Second Run Finished ===\n");
//
//    // 等待CH1反转完成
//    rt_sem_take(&ch1_return_complete_sem, RT_WAITING_FOREVER);
//
//    // 反转回退
//    rt_kprintf("=== CH2 Return Movement ===\n");
//    rt_device_write(u1_dev, 0, "CH1 return completed, CH2 return starting...\r\n", 46);
//    stepper_control(PWM_CH2, CH2_RETURN_REVOLUTIONS, freq_hz, 3);
//
//    // 发送反转完成信号
//    rt_sem_release(&ch2_return_complete_sem);
//    rt_kprintf("=== CH2 Return Movement Finished ===\n");
//
//    rt_kprintf("=== Channel2 Thread Finished ===\n");
//
//    // 发送全部完成信息到PC
//    char all_complete_msg[] = "All motor movements completed! (Including return movements)\r\n";
//    rt_device_write(u1_dev, 0, all_complete_msg, rt_strlen(all_complete_msg));
//}
//
//int main(void)
//{
//    /*============uart test===============*/
//    rt_err_t ret = RT_EOK;
//
//    /* 初始化 UART2（PC→MCU）*/
//    u2_dev = rt_device_find("uart2");
//    if (u2_dev == RT_NULL)
//    {
//        LOG_E("rt_device_find[uart2] failed...\n");
//        return RT_EINVAL;
//    }
//    ret = rt_device_open(u2_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
//    if (ret < 0)
//    {
//        LOG_E("rt_device_open[uart2] failed...\n");
//        return ret;
//    }
//    rt_device_control(u2_dev, RT_DEVICE_CTRL_CONFIG, (void*)&u2_configs);
//    rt_sem_init(&sem2, "rx2", 0, RT_IPC_FLAG_FIFO);
//    rt_device_set_rx_indicate(u2_dev, rx2_callback);
//    rt_thread_t u2_th = rt_thread_create("uart2_r", serial2_thread_entry, RT_NULL, 1024, 10, 5);
//    rt_thread_startup(u2_th);
//
//    /* 初始化 UART1（MCU→PC）*/
//    u1_dev = rt_device_find("uart1");
//    if (u1_dev == RT_NULL)
//    {
//        LOG_E("rt_device_find[uart1] failed...\n");
//        return RT_EINVAL;
//    }
//    ret = rt_device_open(u1_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
//    if (ret < 0)
//    {
//        LOG_E("rt_device_open[uart1] failed...\n");
//        return ret;
//    }
//    rt_device_control(u1_dev, RT_DEVICE_CTRL_CONFIG, (void*)&u2_configs);
//    rt_sem_init(&sem1, "rx1", 0, RT_IPC_FLAG_FIFO);
//    rt_device_set_rx_indicate(u1_dev, rx1_callback);
//    rt_thread_t u1_th = rt_thread_create("uart1_r", serial1_thread_entry, RT_NULL, 1024, 10, 5);
//    rt_thread_startup(u1_th);
//
//    /* 初始消息发送 */
//    const char *msg2 = "UART2 config...\n";
//    rt_device_write(u2_dev, 0, msg2, rt_strlen(msg2));
//    const char *msg1 = "UART1 ready...\n";
//    rt_device_write(u1_dev, 0, msg1, rt_strlen(msg1));
//    /*============uart test===============*/
//
//    // 初始化同步信号量
//    rt_sem_init(&ch1_run1_complete_sem, "ch1_run1", 0, RT_IPC_FLAG_FIFO);
//    rt_sem_init(&ch2_run1_complete_sem, "ch2_run1", 0, RT_IPC_FLAG_FIFO);
//    rt_sem_init(&ch1_run2_complete_sem, "ch1_run2", 0, RT_IPC_FLAG_FIFO);
//    rt_sem_init(&ch2_run2_complete_sem, "ch2_run2", 0, RT_IPC_FLAG_FIFO);
//    rt_sem_init(&ch1_return_complete_sem, "ch1_return", 0, RT_IPC_FLAG_FIFO);
//    rt_sem_init(&ch2_return_complete_sem, "ch2_return", 0, RT_IPC_FLAG_FIFO);
//
//    rt_thread_mdelay(1000);  // 开机延时
//
//    // 发送系统配置信息
//    char config_msg[500];
//    rt_snprintf(config_msg, sizeof(config_msg),
//                "System Config - Dual Run + Return Mode:\r\n"
//                "RUN1: CH1=%d revs(%s), CH2=%d revs(%s)\r\n"
//                "RUN2: CH1=%d revs(%s), CH2=%d revs(%s)\r\n"
//                "RETURN: CH1=%d revs(%s), CH2=%d revs(%s)\r\n"
//                "Interval: %d seconds, Return wait: %d seconds\r\n",
//                CH1_RUN1_REVOLUTIONS, (CH1_RUN1_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD",
//                CH2_RUN1_REVOLUTIONS, (CH2_RUN1_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD",
//                CH1_RUN2_REVOLUTIONS, (CH1_RUN2_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD",
//                CH2_RUN2_REVOLUTIONS, (CH2_RUN2_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD",
//                CH1_RETURN_REVOLUTIONS, (CH1_RETURN_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD",
//                CH2_RETURN_REVOLUTIONS, (CH2_RETURN_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD",
//                RUN_INTERVAL_MS / 1000, RETURN_WAIT_MS / 1000);
//    rt_device_write(u1_dev, 0, config_msg, rt_strlen(config_msg));
//
//    rt_kprintf("Starting dual-run stepper control with return function:\n");
//    rt_kprintf("=== First Run ===\n");
//    rt_kprintf("- Channel1: %d revolutions, %s direction\n",
//               CH1_RUN1_REVOLUTIONS, (CH1_RUN1_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD");
//    rt_kprintf("- Channel2: %d revolutions, %s direction\n",
//               CH2_RUN1_REVOLUTIONS, (CH2_RUN1_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD");
//    rt_kprintf("=== Second Run (after %d seconds) ===\n", RUN_INTERVAL_MS / 1000);
//    rt_kprintf("- Channel1: %d revolutions, %s direction\n",
//               CH1_RUN2_REVOLUTIONS, (CH1_RUN2_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD");
//    rt_kprintf("- Channel2: %d revolutions, %s direction\n",
//               CH2_RUN2_REVOLUTIONS, (CH2_RUN2_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD");
//    rt_kprintf("=== Return Movement (after %d seconds) ===\n", RETURN_WAIT_MS / 1000);
//    rt_kprintf("- Channel1: %d revolutions, %s direction\n",
//               CH1_RETURN_REVOLUTIONS, (CH1_RETURN_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD");
//    rt_kprintf("- Channel2: %d revolutions, %s direction\n",
//               CH2_RETURN_REVOLUTIONS, (CH2_RETURN_DIRECTION == PIN_HIGH) ? "FORWARD" : "BACKWARD");
//
//    // 显示方向控制信息
//    rt_kprintf("Direction Control Configuration:\n");
//    rt_kprintf("- CH1_RUN1_DIRECTION = %s\n", (CH1_RUN1_DIRECTION == PIN_HIGH) ? "PIN_HIGH" : "PIN_LOW");
//    rt_kprintf("- CH2_RUN1_DIRECTION = %s\n", (CH2_RUN1_DIRECTION == PIN_HIGH) ? "PIN_HIGH" : "PIN_LOW");
//    rt_kprintf("- CH1_RUN2_DIRECTION = %s\n", (CH1_RUN2_DIRECTION == PIN_HIGH) ? "PIN_HIGH" : "PIN_LOW");
//    rt_kprintf("- CH2_RUN2_DIRECTION = %s\n", (CH2_RUN2_DIRECTION == PIN_HIGH) ? "PIN_HIGH" : "PIN_LOW");
//    rt_kprintf("- CH1_RETURN_DIRECTION = %s\n", (CH1_RETURN_DIRECTION == PIN_HIGH) ? "PIN_HIGH" : "PIN_LOW");
//    rt_kprintf("- CH2_RETURN_DIRECTION = %s\n", (CH2_RETURN_DIRECTION == PIN_HIGH) ? "PIN_HIGH" : "PIN_LOW");
//
//    /* 创建按顺序执行的步进线程 */
//    rt_thread_t t1 = rt_thread_create("step_ch1", stepper_ch1_thread, RT_NULL,
//                                      1024, RT_THREAD_PRIORITY_MAX - 2, 10);
//    rt_thread_t t2 = rt_thread_create("step_ch2", stepper_ch2_thread, RT_NULL,
//                                      1024, RT_THREAD_PRIORITY_MAX - 2, 10);
//
//    if (t1) {
//        rt_thread_startup(t1);
//        rt_kprintf("Channel1 thread started\n");
//    }
//    if (t2) {
//        rt_thread_startup(t2);
//        rt_kprintf("Channel2 thread started (waiting for CH1)\n");
//    }
//
//    return RT_EOK;
//}
//=====================================================//
//#include <rtthread.h>
//#include <rtdevice.h>
//#include <board.h>
//#include <serial.h>
//#include <rtdbg.h>
//
//#define PWM_DEV_NAME    "pwm2"
//#define PWM_CH1         1
//#define PWM_CH2         2
//#define DIR1_PIN        GET_PIN(G,0)
//#define DIR2_PIN        GET_PIN(G,1)
//
//#define STEPS_PER_REV   200    // 每圈步数，步距角 1.8°
//#define MICROSTEP       16      // 微步倍数
//#define TEST_REVOLUTIONS 9     // 测试转动圈数
//
///*=======================uart test=============================*/
//rt_device_t u2_dev;    // 原 uart2 用于 PC→MCU
//rt_device_t u1_dev;    // 新增 uart1 用于 MCU→PC
//struct rt_semaphore sem2, sem1;
//
//struct serial_configure u2_configs = RT_SERIAL_CONFIG_DEFAULT;
//
///* 中断回调 */
//rt_err_t rx2_callback(rt_device_t dev, rt_size_t size)
//{
//    rt_sem_release(&sem2);
//    return RT_EOK;
//}
//rt_err_t rx1_callback(rt_device_t dev, rt_size_t size)
//{
//    rt_sem_release(&sem1);
//    return RT_EOK;
//}
//
///* UART2 收 PC 发给 MCU 的线程 */
//void serial2_thread_entry(void *parameter)
//{
//    char buffer;
//    while(1)
//    {
//        while (rt_device_read(u2_dev, 0, &buffer, 1) != 1)
//            rt_sem_take(&sem2, RT_WAITING_FOREVER);
//        rt_kprintf("UART2 RX→MCU: %c\n", buffer);
//        /* 回复 MCU 到 PC：通过 UART1 发送 */
//        rt_device_write(u1_dev, 0, &buffer, 1);
//    }
//}
//
///* UART1 收 MCU 数据（可用于调试或桥接） */
//void serial1_thread_entry(void *parameter)
//{
//    char buffer;
//    while(1)
//    {
//        while (rt_device_read(u1_dev, 0, &buffer, 1) != 1)
//            rt_sem_take(&sem1, RT_WAITING_FOREVER);
//        rt_kprintf("UART1 RX←MCU: %c\n", buffer);
//        /* 如果需要，再透传到 PC 上的 UART2 */
//        rt_device_write(u2_dev, 0, &buffer, 1);
//    }
//}
//
///*=======================uart test end=============================*/
//
//static void stepper_thread(void *param)
//{
//    int channel = (int)param;
//    int dir_pin = (channel == PWM_CH1) ? DIR1_PIN : DIR2_PIN;
//    uint32_t revolutions = TEST_REVOLUTIONS;
//    uint32_t freq_hz = 30000;
//
//    struct rt_device_pwm *dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
//    if (!dev)
//    {
//        rt_kprintf("PWM device '%s' not found!\n", PWM_DEV_NAME);
//        return;
//    }
//
//    rt_pwm_enable(dev, channel);
//    rt_pin_mode(dir_pin, PIN_MODE_OUTPUT);
//    rt_pin_write(dir_pin, PIN_LOW);
//
//    uint32_t total_steps = (STEPS_PER_REV * MICROSTEP * revolutions)-50;
//    uint32_t period_ns = 1000000000UL / freq_hz;
//    uint32_t pulse_period_us = period_ns / 1000UL;
//    uint32_t total_time_us = total_steps * pulse_period_us;
//
//    rt_pwm_set(dev, channel, period_ns, period_ns / 2);
//
//    rt_kprintf("CH%d start: %lu rev, freq %lu Hz, time=%lu us\n",
//               channel, revolutions, freq_hz, total_time_us);
//
//    rt_thread_delay(rt_tick_from_millisecond(total_time_us / 1000));
//    rt_pwm_disable(dev, channel);
//    rt_kprintf("CH%d stop\n", channel);
//}
//
//int main(void)
//{
//    /*============uart test===============*/
//    rt_err_t ret = RT_EOK;
//
//    /* 初始化 UART2（PC→MCU）*/
//    u2_dev = rt_device_find("uart2");
//    if (u2_dev == RT_NULL)
//    {
//        LOG_E("rt_device_find[uart2] failed...\n");
//        return RT_EINVAL;
//    }
//    ret = rt_device_open(u2_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
//    if (ret < 0)
//    {
//        LOG_E("rt_device_open[uart2] failed...\n");
//        return ret;
//    }
//    rt_device_control(u2_dev, RT_DEVICE_CTRL_CONFIG, (void*)&u2_configs);
//    rt_sem_init(&sem2, "rx2", 0, RT_IPC_FLAG_FIFO);
//    rt_device_set_rx_indicate(u2_dev, rx2_callback);
//    rt_thread_t u2_th = rt_thread_create("uart2_r", serial2_thread_entry, RT_NULL, 1024, 10, 5);
//    rt_thread_startup(u2_th);
//
//    /* 初始化 UART1（MCU→PC）*/
//    u1_dev = rt_device_find("uart1");
//    if (u1_dev == RT_NULL)
//    {
//        LOG_E("rt_device_find[uart1] failed...\n");
//        return RT_EINVAL;
//    }
//    ret = rt_device_open(u1_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
//    if (ret < 0)
//    {
//        LOG_E("rt_device_open[uart1] failed...\n");
//        return ret;
//    }
//    rt_device_control(u1_dev, RT_DEVICE_CTRL_CONFIG, (void*)&u2_configs);
//    rt_sem_init(&sem1, "rx1", 0, RT_IPC_FLAG_FIFO);
//    rt_device_set_rx_indicate(u1_dev, rx1_callback);
//    rt_thread_t u1_th = rt_thread_create("uart1_r", serial1_thread_entry, RT_NULL, 1024, 10, 5);
//    rt_thread_startup(u1_th);
//
//    /* 初始消息发送 */
//    const char *msg2 = "UART2 config...\n";
//    rt_device_write(u2_dev, 0, msg2, rt_strlen(msg2));
//    const char *msg1 = "UART1 ready...\n";
//    rt_device_write(u1_dev, 0, msg1, rt_strlen(msg1));
//    /*============uart test===============*/
//
//    rt_thread_mdelay(1000);  // 开机延时
//
//    /* 保留原有步进线程 */
//    rt_thread_t t1 = rt_thread_create("step1", stepper_thread, (void *)PWM_CH1,
//                                      512, RT_THREAD_PRIORITY_MAX - 2, 10);
//    rt_thread_t t2 = rt_thread_create("step2", stepper_thread, (void *)PWM_CH2,
//                                      512, RT_THREAD_PRIORITY_MAX - 2, 10);
//    if (t1) rt_thread_startup(t1);
//    if (t2) rt_thread_startup(t2);
//
//    return RT_EOK;
//}

//#include <rtthread.h>
//#include <rtdevice.h>
//#include <board.h>
//#include <serial.h>
//#include <rtdbg.h>
//#include <string.h>
//
//#define PWM_DEV_NAME    "pwm2"
//#define PWM_CH1         1
//#define PWM_CH2         2
//#define DIR1_PIN        GET_PIN(G,0)
//#define DIR2_PIN        GET_PIN(G,1)
//
//#define STEPS_PER_REV   200    // 每圈步数，步距角 1.8°
//#define MICROSTEP       4      // 微步倍数
//#define TEST_REVOLUTIONS 4     // 测试转动圈数
//
///*=======================新的串口测试部分=============================*/
//#define UART_RX_NAME "uart2" // OpenMV → MCU
//#define UART_TX_NAME "uart1" // MCU → PC
//#define DATA_LEN 8
//
//static rt_device_t uart_rx_dev = RT_NULL;
//static rt_device_t uart_tx_dev = RT_NULL;
//static struct rt_semaphore sem_rx;
//
//// 校验函数：第1位必须是x，第8位（最后一位）必须是y
//static rt_bool_t check_xy_format(const char *buf)
//{
//    if (!buf)
//        return RT_FALSE;
//    if (buf[0] != 'x')
//        return RT_FALSE;
//    if (buf[7] != 'y')
//        return RT_FALSE;
//    return RT_TRUE;
//}
//
//// 寻找下一个'x'字符，返回找到的'x'
//static char find_next_x(rt_device_t dev, struct rt_semaphore *sem)
//{
//    char ch;
//    while (1)
//    {
//        int ret = rt_device_read(dev, 0, &ch, 1);
//        if (ret == 1)
//        {
//            if (ch == 'x')
//            {
//                return ch; // 返回找到的'x'
//            }
//            // 否则继续丢弃
//        }
//        else
//        {
//            rt_sem_take(sem, RT_WAITING_FOREVER);
//        }
//    }
//}
//
///* UART2接收线程：接收OpenMV数据并转发到PC */
//static void uart_rx_thread(void *param)
//{
//    char buf[DATA_LEN];
//    char xy[DATA_LEN + 1]; // 8字节+结尾符
//
//    rt_kprintf("UART RX thread started, waiting for OpenMV data...\n");
//
//    while (1)
//    {
//        // 寻找第一个'x'
//        buf[0] = find_next_x(uart_rx_dev, &sem_rx);
//        int read_size = 1;
//
//        // 读取剩余的7个字节
//        while (read_size < DATA_LEN)
//        {
//            int ret = rt_device_read(uart_rx_dev, 0, buf + read_size, DATA_LEN - read_size);
//            if (ret > 0)
//            {
//                read_size += ret;
//            }
//            else
//            {
//                rt_sem_take(&sem_rx, RT_WAITING_FOREVER);
//            }
//        }
//
//        // 校验：首位x，第8位y
//        if (check_xy_format(buf))
//        {
//            memcpy(xy, buf, DATA_LEN);
//            xy[DATA_LEN] = '\0';
//
//            rt_kprintf("Valid data received: %s\n", xy);
//
//            // 转发到PC (通过UART1)
//            rt_device_write(uart_tx_dev, 0, "OpenMV: ", 8);
//            rt_device_write(uart_tx_dev, 0, xy, DATA_LEN);
//            rt_device_write(uart_tx_dev, 0, "\r\n", 2);
//
//            rt_kprintf("Data forwarded to PC\n");
//        }
//        else
//        {
//            rt_kprintf("Invalid data format, discarding\n");
//        }
//    }
//}
//
//static rt_err_t rx_callback(rt_device_t dev, rt_size_t size)
//{
//    rt_sem_release(&sem_rx);
//    return RT_EOK;
//}
//
///*=======================新的串口测试部分结束=============================*/
//
//// 步进电机线程（保留原有逻辑）
//static void stepper_thread(void *param)
//{
//    int channel = (int)param;
//    int dir_pin = (channel == PWM_CH1) ? DIR1_PIN : DIR2_PIN;
//    uint32_t revolutions = TEST_REVOLUTIONS;
//    uint32_t freq_hz = 30000;
//
//    struct rt_device_pwm *dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
//    if (!dev)
//    {
//        rt_kprintf("PWM device '%s' not found!\n", PWM_DEV_NAME);
//        return;
//    }
//
//    rt_pwm_enable(dev, channel);
//    rt_pin_mode(dir_pin, PIN_MODE_OUTPUT);
//    rt_pin_write(dir_pin, PIN_LOW);
//
//    uint32_t total_steps = (STEPS_PER_REV * MICROSTEP * revolutions) - 50;
//    uint32_t period_ns = 1000000000UL / freq_hz;
//    uint32_t pulse_period_us = period_ns / 1000UL;
//    uint32_t total_time_us = total_steps * pulse_period_us;
//
//    rt_pwm_set(dev, channel, period_ns, period_ns / 2);
//
//    rt_kprintf("CH%d start: %lu rev, freq %lu Hz, time=%lu us\n",
//               channel, revolutions, freq_hz, total_time_us);
//
//    rt_thread_delay(rt_tick_from_millisecond(total_time_us / 1000));
//    rt_pwm_disable(dev, channel);
//    rt_kprintf("CH%d stop\n", channel);
//}
//
//int main(void)
//{
//    /*============新的串口初始化===============*/
//    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
//    cfg.baud_rate = BAUD_RATE_115200;
//
//    rt_kprintf("Starting UART bridge application...\n");
//
//    // 配置UART2 (OpenMV → MCU) - 接收模式
//    uart_rx_dev = rt_device_find(UART_RX_NAME);
//    if (!uart_rx_dev)
//    {
//        rt_kprintf("ERROR: Cannot find %s\n", UART_RX_NAME);
//        return -1;
//    }
//
//    rt_device_open(uart_rx_dev, RT_DEVICE_FLAG_INT_RX);
//    rt_device_control(uart_rx_dev, RT_DEVICE_CTRL_CONFIG, &cfg);
//    rt_sem_init(&sem_rx, "sem_rx", 0, RT_IPC_FLAG_FIFO);
//    rt_device_set_rx_indicate(uart_rx_dev, rx_callback);
//    rt_kprintf("UART2 configured for OpenMV input (115200 baud)\n");
//
//    // 配置UART1 (MCU → PC) - 读写模式
//    uart_tx_dev = rt_device_find(UART_TX_NAME);
//    if (!uart_tx_dev)
//    {
//        rt_kprintf("ERROR: Cannot find %s\n", UART_TX_NAME);
//        return -1;
//    }
//
//    rt_device_open(uart_tx_dev, RT_DEVICE_FLAG_RDWR);
//    rt_device_control(uart_tx_dev, RT_DEVICE_CTRL_CONFIG, &cfg);
//    rt_kprintf("UART1 configured for PC output (115200 baud)\n");
//
//    // 创建接收线程
//    rt_thread_t rx_t = rt_thread_create("uart_rx", uart_rx_thread, RT_NULL, 2048, 15, 10);
//    if (!rx_t)
//    {
//        rt_kprintf("ERROR: Failed to create UART RX thread\n");
//        return -1;
//    }
//    rt_thread_startup(rx_t);
//    rt_kprintf("UART RX thread started\n");
//
//    // 发送就绪消息到PC
//    rt_device_write(uart_tx_dev, 0, "UART bridge ready - OpenMV to PC\r\n", 35);
//    rt_kprintf("Bridge ready, waiting for OpenMV data...\n");
//
//    /*============新的串口初始化结束===============*/
//
//    rt_thread_mdelay(1000);  // 开机延时
//
//    /* 保留原有步进线程 */
//    rt_thread_t t1 = rt_thread_create("step1", stepper_thread, (void *)PWM_CH1,
//                                      512, RT_THREAD_PRIORITY_MAX - 2, 10);
//    rt_thread_t t2 = rt_thread_create("step2", stepper_thread, (void *)PWM_CH2,
//                                      512, RT_THREAD_PRIORITY_MAX - 2, 10);
//    if (t1) rt_thread_startup(t1);
//    if (t2) rt_thread_startup(t2);
//
//    return RT_EOK;
//}


//=================version1.0电机代码=================//
//#include <rtthread.h>
//#include <rtdevice.h>
//#include <board.h>
//#include <serial.h>
//#include <rtdbg.h>
//#include <string.h>
//#include <stdlib.h>
//#include <stdio.h>
//
//#define PWM_DEV_NAME    "pwm2"
//#define PWM_CH1         1      // X轴电机
//#define PWM_CH2         2      // Y轴电机
//#define DIR1_PIN        GET_PIN(G,0)  // X轴方向控制
//#define DIR2_PIN        GET_PIN(G,1)  // Y轴方向控制
//
//#define STEPS_PER_REV   200    // 每圈步数，步距角 1.8°
//#define MICROSTEP       4      // 微步倍数
//#define LEAD_SCREW_PITCH 8.0   // 丝杆导程 8mm
//#define MOTOR_FREQ_HZ   10000  // 电机运行频率
//
///*=======================串口通信部分=============================*/
//#define UART_RX_NAME "uart2" // OpenMV → MCU
//#define UART_TX_NAME "uart1" // MCU → PC
//#define DATA_LEN 8
//
//static rt_device_t uart_rx_dev = RT_NULL;
//static rt_device_t uart_tx_dev = RT_NULL;
//static struct rt_semaphore sem_rx;
//
//// 电机控制结构体
//typedef struct {
//    int channel;
//    int dir_pin;
//    float current_position_mm;
//    rt_bool_t is_moving;
//} motor_control_t;
//
//static motor_control_t x_motor = {PWM_CH1, DIR1_PIN, 0.0, RT_FALSE};
//static motor_control_t y_motor = {PWM_CH2, DIR2_PIN, 0.0, RT_FALSE};
//
//// 坐标数据结构
//typedef struct {
//    int x;
//    int y;
//} coordinate_t;
//
//// 校验函数：第1位必须是x，第8位（最后一位）必须是y
//static rt_bool_t check_xy_format(const char *buf)
//{
//    if (!buf)
//        return RT_FALSE;
//    if (buf[0] != 'x')
//        return RT_FALSE;
//    if (buf[7] != 'y')
//        return RT_FALSE;
//    return RT_TRUE;
//}
//
//// 解析XY坐标
//static rt_bool_t parse_xy_coordinate(const char *buf, coordinate_t *coord)
//{
//    if (!buf || !coord) return RT_FALSE;
//
//    char x_str[4] = {0};
//    char y_str[4] = {0};
//
//    // 提取X坐标 (位置1-3)
//    strncpy(x_str, &buf[1], 3);
//    x_str[3] = '\0';
//
//    // 提取Y坐标 (位置4-6)
//    strncpy(y_str, &buf[4], 3);
//    y_str[3] = '\0';
//
//    coord->x = atoi(x_str);
//    coord->y = atoi(y_str);
//
//    rt_kprintf("Parsed coordinates: X=%d, Y=%d\n", coord->x, coord->y);
//
//    return RT_TRUE;
//}
//
//// 寻找下一个'x'字符
//static char find_next_x(rt_device_t dev, struct rt_semaphore *sem)
//{
//    char ch;
//    while (1)
//    {
//        int ret = rt_device_read(dev, 0, &ch, 1);
//        if (ret == 1)
//        {
//            if (ch == 'x')
//            {
//                return ch;
//            }
//        }
//        else
//        {
//            rt_sem_take(sem, RT_WAITING_FOREVER);
//        }
//    }
//}
//
///*=======================电机控制部分=============================*/
//
//// 计算移动距离对应的步数
//static uint32_t calculate_steps_for_distance(float distance_mm)
//{
//    float revolutions = distance_mm / LEAD_SCREW_PITCH;
//    uint32_t steps = (uint32_t)(revolutions * STEPS_PER_REV * MICROSTEP);
//    return steps;
//}
//
//// 控制单个电机移动
//static void move_motor(motor_control_t *motor, float target_mm)
//{
//    if (motor->is_moving) {
//        rt_kprintf("Motor CH%d is already moving, skipping\n", motor->channel);
//        return;
//    }
//
//    float distance = target_mm - motor->current_position_mm;
//
//    // 简单的浮点比较
//    if (distance < 0.1 && distance > -0.1) {
//        rt_kprintf("Motor CH%d already at target position %.2fmm\n", motor->channel, target_mm);
//        return;
//    }
//
//    motor->is_moving = RT_TRUE;
//
//    struct rt_device_pwm *dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
//    if (!dev) {
//        rt_kprintf("PWM device not found!\n");
//        motor->is_moving = RT_FALSE;
//        return;
//    }
//
//    // 设置方向
//    rt_pin_mode(motor->dir_pin, PIN_MODE_OUTPUT);
//    if (distance > 0) {
//        rt_pin_write(motor->dir_pin, PIN_LOW);  // 正方向
//    } else {
//        rt_pin_write(motor->dir_pin, PIN_HIGH);   // 负方向
//        distance = -distance;  // 取绝对值
//    }
//
//    uint32_t steps = calculate_steps_for_distance(distance);
//    uint32_t period_ns = 1000000000UL / MOTOR_FREQ_HZ;
//    uint32_t move_time_ms = (steps * period_ns) / 1000000UL;
//
//    rt_kprintf("Motor CH%d moving %.2fmm (%lu steps, %lums)\n",
//               motor->channel, distance, steps, move_time_ms);
//
//    // 启动PWM
//    rt_pwm_set(dev, motor->channel, period_ns, period_ns / 2);
//    rt_pwm_enable(dev, motor->channel);
//
//    // 运行指定时间
//    rt_thread_delay(rt_tick_from_millisecond(move_time_ms));
//
//    // 停止PWM
//    rt_pwm_disable(dev, motor->channel);
//
//    // 更新当前位置
//    motor->current_position_mm = target_mm;
//    motor->is_moving = RT_FALSE;
//
//    rt_kprintf("Motor CH%d movement complete, now at %.2fmm\n",
//               motor->channel, motor->current_position_mm);
//}
//
//// 控制XY电机移动
//static void move_to_coordinate(coordinate_t *coord)
//{
//    // 将像素坐标转换为实际距离（mm）
//    // 根据你的实际系统调整这个比例
//    float x_target_mm = coord->x * 0.1;  // 1像素 = 0.1mm，可调整
//    float y_target_mm = coord->y * 0.1;
//
//    rt_kprintf("Target position: X=%.2fmm, Y=%.2fmm\n", x_target_mm, y_target_mm);
//
//    // 发送开始移动信息到PC
//    char start_msg[80];
//    rt_snprintf(start_msg, sizeof(start_msg), "Moving to position: X=%.2f, Y=%.2f\r\n", x_target_mm, y_target_mm);
//    rt_device_write(uart_tx_dev, 0, start_msg, rt_strlen(start_msg));
//
//    // 控制X轴电机
//    move_motor(&x_motor, x_target_mm);
//
//    // 控制Y轴电机
//    move_motor(&y_motor, y_target_mm);
//
//    // 发送完成信息到PC
//    char complete_msg[80];
//    rt_snprintf(complete_msg, sizeof(complete_msg), "Movement complete: X=%.2f, Y=%.2f\r\n", x_target_mm, y_target_mm);
//    rt_device_write(uart_tx_dev, 0, complete_msg, rt_strlen(complete_msg));
//
//    rt_kprintf("Coordinate movement completed\n");
//}
//
///* UART2接收线程：接收OpenMV数据并直接控制电机 */
//static void uart_rx_thread(void *param)
//{
//    char buf[DATA_LEN];
//    char xy[DATA_LEN + 1];
//    coordinate_t coord;
//
//    rt_kprintf("UART RX thread started, waiting for OpenMV data...\n");
//
//    while (1)
//    {
//        // 寻找第一个'x'
//        buf[0] = find_next_x(uart_rx_dev, &sem_rx);
//        int read_size = 1;
//
//        // 读取剩余的7个字节
//        while (read_size < DATA_LEN)
//        {
//            int ret = rt_device_read(uart_rx_dev, 0, buf + read_size, DATA_LEN - read_size);
//            if (ret > 0)
//            {
//                read_size += ret;
//            }
//            else
//            {
//                rt_sem_take(&sem_rx, RT_WAITING_FOREVER);
//            }
//        }
//
//        // 校验并解析坐标
//        if (check_xy_format(buf))
//        {
//            memcpy(xy, buf, DATA_LEN);
//            xy[DATA_LEN] = '\0';
//
//            rt_kprintf("Valid data received: %s\n", xy);
//
//            // 解析坐标
//            if (parse_xy_coordinate(buf, &coord))
//            {
//                // 直接移动到目标位置
//                move_to_coordinate(&coord);
//            }
//
//            // 转发原始数据到PC
//            rt_device_write(uart_tx_dev, 0, "Received: ", 10);
//            rt_device_write(uart_tx_dev, 0, xy, DATA_LEN);
//            rt_device_write(uart_tx_dev, 0, "\r\n", 2);
//        }
//        else
//        {
//            rt_kprintf("Invalid data format, discarding\n");
//        }
//    }
//}
//
//static rt_err_t rx_callback(rt_device_t dev, rt_size_t size)
//{
//    rt_sem_release(&sem_rx);
//    return RT_EOK;
//}
//
//int main(void)
//{
//    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
//    cfg.baud_rate = BAUD_RATE_115200;
//
//    rt_kprintf("Starting simple XY-controlled stepper motor system...\n");
//
//    // 配置UART2 (OpenMV → MCU)
//    uart_rx_dev = rt_device_find(UART_RX_NAME);
//    if (!uart_rx_dev)
//    {
//        rt_kprintf("ERROR: Cannot find %s\n", UART_RX_NAME);
//        return -1;
//    }
//
//    rt_device_open(uart_rx_dev, RT_DEVICE_FLAG_INT_RX);
//    rt_device_control(uart_rx_dev, RT_DEVICE_CTRL_CONFIG, &cfg);
//    rt_sem_init(&sem_rx, "sem_rx", 0, RT_IPC_FLAG_FIFO);
//    rt_device_set_rx_indicate(uart_rx_dev, rx_callback);
//    rt_kprintf("UART2 configured for OpenMV input\n");
//
//    // 配置UART1 (MCU → PC)
//    uart_tx_dev = rt_device_find(UART_TX_NAME);
//    if (!uart_tx_dev)
//    {
//        rt_kprintf("ERROR: Cannot find %s\n", UART_TX_NAME);
//        return -1;
//    }
//
//    rt_device_open(uart_tx_dev, RT_DEVICE_FLAG_RDWR);
//    rt_device_control(uart_tx_dev, RT_DEVICE_CTRL_CONFIG, &cfg);
//    rt_kprintf("UART1 configured for PC output\n");
//
//    // 创建接收线程
//    rt_thread_t rx_t = rt_thread_create("uart_rx", uart_rx_thread, RT_NULL, 2048, 15, 10);
//    if (!rx_t)
//    {
//        rt_kprintf("ERROR: Failed to create UART RX thread\n");
//        return -1;
//    }
//    rt_thread_startup(rx_t);
//
//    rt_device_write(uart_tx_dev, 0, "Simple XY stepper control system ready\r\n", 41);
//    rt_kprintf("System ready - single coordinate recognition and movement\n");
//
//    return RT_EOK;
//}

//======================================================================//
//=================version2.0电机代码=================//
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <serial.h>
#include <rtdbg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define PWM_DEV_NAME    "pwm2"
#define PWM_CH1         1      // X轴电机
#define PWM_CH2         2      // Y轴电机
#define DIR1_PIN        GET_PIN(G,0)  // X轴方向控制
#define DIR2_PIN        GET_PIN(G,1)  // Y轴方向控制

#define STEPS_PER_REV   200    // 每圈步数，步距角 1.8°
#define MICROSTEP       4      // 微步倍数
#define LEAD_SCREW_PITCH 8.0   // 丝杆导程 8mm
#define MOTOR_FREQ_HZ   8000   // 降低频率，提高精度

// 校准参数 - 根据65cm高度调整
#define CAMERA_HEIGHT_CM    65.0    // 摄像头高度
#define PIXEL_TO_MM_RATIO   0.5     // 像素到毫米的转换比例，需要实际测量校准
#define X_DIRECTION_INVERT  RT_FALSE // X轴方向是否反向
#define Y_DIRECTION_INVERT  RT_FALSE // Y轴方向是否反向

// 图像中心偏移校准（如果摄像头不在正中心）
#define IMAGE_CENTER_X_OFFSET  0    // 图像中心X偏移
#define IMAGE_CENTER_Y_OFFSET  0    // 图像中心Y偏移

/*=======================串口通信部分=============================*/
#define UART_RX_NAME "uart2" // OpenMV → MCU
#define UART_TX_NAME "uart1" // MCU → PC
#define DATA_LEN 8

static rt_device_t uart_rx_dev = RT_NULL;
static rt_device_t uart_tx_dev = RT_NULL;
static struct rt_semaphore sem_rx;

// 电机控制结构体
typedef struct {
    int channel;
    int dir_pin;
    float current_position_mm;
    rt_bool_t is_moving;
    rt_bool_t direction_inverted;  // 方向是否反向
} motor_control_t;

static motor_control_t x_motor = {PWM_CH1, DIR1_PIN, 0.0, RT_FALSE, X_DIRECTION_INVERT};
static motor_control_t y_motor = {PWM_CH2, DIR2_PIN, 0.0, RT_FALSE, Y_DIRECTION_INVERT};

// 坐标数据结构
typedef struct {
    int x;
    int y;
} coordinate_t;

// 校验函数：第1位必须是x，第8位（最后一位）必须是y
static rt_bool_t check_xy_format(const char *buf)
{
    if (!buf)
        return RT_FALSE;
    if (buf[0] != 'x')
        return RT_FALSE;
    if (buf[7] != 'y')
        return RT_FALSE;
    return RT_TRUE;
}

// 解析XY坐标
static rt_bool_t parse_xy_coordinate(const char *buf, coordinate_t *coord)
{
    if (!buf || !coord) return RT_FALSE;

    char x_str[4] = {0};
    char y_str[4] = {0};

    // 提取X坐标 (位置1-3)
    strncpy(x_str, &buf[1], 3);
    x_str[3] = '\0';

    // 提取Y坐标 (位置4-6)
    strncpy(y_str, &buf[4], 3);
    y_str[3] = '\0';

    coord->x = atoi(x_str);
    coord->y = atoi(y_str);

    rt_kprintf("Raw coordinates: X=%d, Y=%d\n", coord->x, coord->y);

    return RT_TRUE;
}

// 坐标校准函数 - 考虑图像中心和实际距离转换
static void calibrate_coordinates(coordinate_t *raw_coord, float *x_mm, float *y_mm)
{
    // 应用中心偏移
    int corrected_x = raw_coord->x + IMAGE_CENTER_X_OFFSET;
    int corrected_y = raw_coord->y + IMAGE_CENTER_Y_OFFSET;

    // 转换为实际距离（毫米）
    *x_mm = corrected_x * PIXEL_TO_MM_RATIO;
    *y_mm = corrected_y * PIXEL_TO_MM_RATIO;

    rt_kprintf("Calibrated position: X=%.2fmm, Y=%.2fmm\n", *x_mm, *y_mm);

    // 发送校准信息到PC
    char calib_msg[100];
    rt_snprintf(calib_msg, sizeof(calib_msg),
                "Calibration: Raw(%d,%d) -> Real(%.2f,%.2f)mm\r\n",
                raw_coord->x, raw_coord->y, *x_mm, *y_mm);
    rt_device_write(uart_tx_dev, 0, calib_msg, rt_strlen(calib_msg));
}

// 寻找下一个'x'字符
static char find_next_x(rt_device_t dev, struct rt_semaphore *sem)
{
    char ch;
    while (1)
    {
        int ret = rt_device_read(dev, 0, &ch, 1);
        if (ret == 1)
        {
            if (ch == 'x')
            {
                return ch;
            }
        }
        else
        {
            rt_sem_take(sem, RT_WAITING_FOREVER);
        }
    }
}

/*=======================电机控制部分=============================*/

// 计算移动距离对应的步数
static uint32_t calculate_steps_for_distance(float distance_mm)
{
    float revolutions = distance_mm / LEAD_SCREW_PITCH;
    uint32_t steps = (uint32_t)(revolutions * STEPS_PER_REV * MICROSTEP);
    return steps;
}

// 控制单个电机移动 - 改进的方向控制
static void move_motor(motor_control_t *motor, float target_mm)
{
    if (motor->is_moving) {
        rt_kprintf("Motor CH%d is already moving, skipping\n", motor->channel);
        return;
    }

    float distance = target_mm - motor->current_position_mm;

    // 精度检查
    if (distance < 0.5 && distance > -0.5) {
        rt_kprintf("Motor CH%d already at target position %.2fmm\n", motor->channel, target_mm);
        return;
    }

    motor->is_moving = RT_TRUE;

    struct rt_device_pwm *dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
    if (!dev) {
        rt_kprintf("PWM device not found!\n");
        motor->is_moving = RT_FALSE;
        return;
    }

    // 设置方向 - 考虑方向反转
    rt_pin_mode(motor->dir_pin, PIN_MODE_OUTPUT);
    rt_bool_t forward_direction = (distance > 0);

    // 如果该轴设置为反向，则反转方向信号
    if (motor->direction_inverted) {
        forward_direction = !forward_direction;
    }

    if (forward_direction) {
        rt_pin_write(motor->dir_pin, PIN_HIGH);
        rt_kprintf("Motor CH%d direction: FORWARD (HIGH)\n", motor->channel);
    } else {
        rt_pin_write(motor->dir_pin, PIN_LOW);
        rt_kprintf("Motor CH%d direction: BACKWARD (LOW)\n", motor->channel);
    }

    distance = (distance > 0) ? distance : -distance;  // 取绝对值

    uint32_t steps = calculate_steps_for_distance(distance);
    uint32_t period_ns = 1000000000UL / MOTOR_FREQ_HZ;
    uint32_t move_time_ms = (steps * period_ns) / 1000000UL;

    rt_kprintf("Motor CH%d moving %.2fmm (%lu steps, %lums)\n",
               motor->channel, distance, steps, move_time_ms);

    // 发送电机移动信息到PC
    char motor_msg[100];
    rt_snprintf(motor_msg, sizeof(motor_msg),
                "Motor CH%d: %.2fmm, %lu steps, %lums\r\n",
                motor->channel, distance, steps, move_time_ms);
    rt_device_write(uart_tx_dev, 0, motor_msg, rt_strlen(motor_msg));

    // 启动PWM
    rt_pwm_set(dev, motor->channel, period_ns, period_ns / 2);
    rt_pwm_enable(dev, motor->channel);

    // 运行指定时间
    rt_thread_delay(rt_tick_from_millisecond(move_time_ms));

    // 停止PWM
    rt_pwm_disable(dev, motor->channel);

    // 更新当前位置
    motor->current_position_mm = target_mm;
    motor->is_moving = RT_FALSE;

    rt_kprintf("Motor CH%d movement complete, now at %.2fmm\n",
               motor->channel, motor->current_position_mm);
}

// 控制XY电机移动
static void move_to_coordinate(coordinate_t *coord)
{
    float x_target_mm, y_target_mm;

    // 使用校准函数转换坐标
    calibrate_coordinates(coord, &x_target_mm, &y_target_mm);

    rt_kprintf("Target position: X=%.2fmm, Y=%.2fmm\n", x_target_mm, y_target_mm);

    // 发送开始移动信息到PC
    char start_msg[100];
    rt_snprintf(start_msg, sizeof(start_msg),
                "Moving to calibrated position: X=%.2f, Y=%.2f\r\n",
                x_target_mm, y_target_mm);
    rt_device_write(uart_tx_dev, 0, start_msg, rt_strlen(start_msg));

    // 控制X轴电机（Channel 1）
    rt_kprintf("=== Moving X-axis ===\n");
    move_motor(&x_motor, x_target_mm);

    // 控制Y轴电机（Channel 2）
    rt_kprintf("=== Moving Y-axis ===\n");
    move_motor(&y_motor, y_target_mm);

    // 发送完成信息到PC
    char complete_msg[100];
    rt_snprintf(complete_msg, sizeof(complete_msg),
                "Movement complete: X=%.2f, Y=%.2f\r\n",
                x_target_mm, y_target_mm);
    rt_device_write(uart_tx_dev, 0, complete_msg, rt_strlen(complete_msg));

    rt_kprintf("=== Coordinate movement completed ===\n");
}

/* UART2接收线程：接收OpenMV数据并直接控制电机 */
static void uart_rx_thread(void *param)
{
    char buf[DATA_LEN];
    char xy[DATA_LEN + 1];
    coordinate_t coord;

    rt_kprintf("UART RX thread started, waiting for OpenMV data...\n");
    rt_kprintf("Camera height: %.1fcm, Pixel ratio: %.2f\n", CAMERA_HEIGHT_CM, PIXEL_TO_MM_RATIO);

    while (1)
    {
        // 寻找第一个'x'
        buf[0] = find_next_x(uart_rx_dev, &sem_rx);
        int read_size = 1;

        // 读取剩余的7个字节
        while (read_size < DATA_LEN)
        {
            int ret = rt_device_read(uart_rx_dev, 0, buf + read_size, DATA_LEN - read_size);
            if (ret > 0)
            {
                read_size += ret;
            }
            else
            {
                rt_sem_take(&sem_rx, RT_WAITING_FOREVER);
            }
        }

        // 校验并解析坐标
        if (check_xy_format(buf))
        {
            memcpy(xy, buf, DATA_LEN);
            xy[DATA_LEN] = '\0';

            rt_kprintf("Valid data received: %s\n", xy);

            // 解析坐标
            if (parse_xy_coordinate(buf, &coord))
            {
                // 直接移动到目标位置
                move_to_coordinate(&coord);
            }

            // 转发原始数据到PC
            rt_device_write(uart_tx_dev, 0, "Received: ", 10);
            rt_device_write(uart_tx_dev, 0, xy, DATA_LEN);
            rt_device_write(uart_tx_dev, 0, "\r\n", 2);
        }
        else
        {
            rt_kprintf("Invalid data format, discarding\n");
        }
    }
}

static rt_err_t rx_callback(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&sem_rx);
    return RT_EOK;
}

int main(void)
{
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
    cfg.baud_rate = BAUD_RATE_115200;

    rt_kprintf("Starting calibrated XY-controlled stepper motor system...\n");
    rt_kprintf("Configuration:\n");
    rt_kprintf("- Camera height: %.1fcm\n", CAMERA_HEIGHT_CM);
    rt_kprintf("- Pixel to mm ratio: %.3f\n", PIXEL_TO_MM_RATIO);
    rt_kprintf("- X-axis direction inverted: %s\n", X_DIRECTION_INVERT ? "YES" : "NO");
    rt_kprintf("- Y-axis direction inverted: %s\n", Y_DIRECTION_INVERT ? "YES" : "NO");
    rt_kprintf("- Lead screw pitch: %.1fmm\n", LEAD_SCREW_PITCH);

    // 配置UART2 (OpenMV → MCU)
    uart_rx_dev = rt_device_find(UART_RX_NAME);
    if (!uart_rx_dev)
    {
        rt_kprintf("ERROR: Cannot find %s\n", UART_RX_NAME);
        return -1;
    }

    rt_device_open(uart_rx_dev, RT_DEVICE_FLAG_INT_RX);
    rt_device_control(uart_rx_dev, RT_DEVICE_CTRL_CONFIG, &cfg);
    rt_sem_init(&sem_rx, "sem_rx", 0, RT_IPC_FLAG_FIFO);
    rt_device_set_rx_indicate(uart_rx_dev, rx_callback);
    rt_kprintf("UART2 configured for OpenMV input\n");

    // 配置UART1 (MCU → PC)
    uart_tx_dev = rt_device_find(UART_TX_NAME);
    if (!uart_tx_dev)
    {
        rt_kprintf("ERROR: Cannot find %s\n", UART_TX_NAME);
        return -1;
    }

    rt_device_open(uart_tx_dev, RT_DEVICE_FLAG_RDWR);
    rt_device_control(uart_tx_dev, RT_DEVICE_CTRL_CONFIG, &cfg);
    rt_kprintf("UART1 configured for PC output\n");

    // 创建接收线程
    rt_thread_t rx_t = rt_thread_create("uart_rx", uart_rx_thread, RT_NULL, 2048, 15, 10);
    if (!rx_t)
    {
        rt_kprintf("ERROR: Failed to create UART RX thread\n");
        return -1;
    }
    rt_thread_startup(rx_t);

    // 发送系统就绪信息
    char ready_msg[150];
    rt_snprintf(ready_msg, sizeof(ready_msg),
                "Calibrated XY system ready - Height:%.1fcm, Ratio:%.3f\r\n",
                CAMERA_HEIGHT_CM, PIXEL_TO_MM_RATIO);
    rt_device_write(uart_tx_dev, 0, ready_msg, rt_strlen(ready_msg));

    rt_kprintf("System ready - calibrated for 65cm camera height\n");

    return RT_EOK;
}
//===================================================//



/*===========循环旋转测试代码=============*/
//#include <rtthread.h>
//#include <rtdevice.h>
//#include <board.h>
//#include <stm32f4xx_hal.h>
//
//#define PWM_DEV_NAME   "pwm2"
//#define PWM_CH1        1
//#define PWM_CH2        2
//#define DIR1_PIN       GET_PIN(G, 0)
//#define DIR2_PIN       GET_PIN(G, 1)
//
//static struct rt_device_pwm *pwm_dev = RT_NULL;
//
//static void dual_stepper_thread(void *parameter)
//{
//    int dir1 = 0, dir2 = 0;
//    uint32_t freq, period_ns, pulse_ns;
//
//    /* 初始化方向 GPIO */
//    rt_pin_mode(DIR1_PIN, PIN_MODE_OUTPUT);
//    rt_pin_mode(DIR2_PIN, PIN_MODE_OUTPUT);
//
//    /* 查找并初始化 PWM 设备 */
//    pwm_dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
//    if (pwm_dev == RT_NULL)
//    {
//        rt_kprintf("Error: PWM device '%s' not found\n", PWM_DEV_NAME);
//        return;
//    }
//
//    /* 启用两个 PWM 通道 */
//    rt_pwm_enable(pwm_dev, PWM_CH1);
//    rt_pwm_enable(pwm_dev, PWM_CH2);
//
//    while (1)
//    {
//        /* 改变配置 */
//        dir1 ^= 1;
//        dir2 ^= 1;
//        freq = dir1 ? 5000 : 20000;  /* 5?kHz ? 20?kHz 切换 */
//
//        rt_pin_write(DIR1_PIN, dir1 ? PIN_HIGH : PIN_LOW);
//        rt_pin_write(DIR2_PIN, dir2 ? PIN_HIGH : PIN_LOW);
//
//        /* 计算 PWM 周期 ns 与占空比 50% */
//        period_ns = 1000000000UL / freq;
//        pulse_ns = period_ns / 2;
//
//        /* 设置 PWM 输出 */
//        rt_pwm_set(pwm_dev, PWM_CH1, period_ns, pulse_ns);
//        rt_pwm_set(pwm_dev, PWM_CH2, period_ns, pulse_ns);
//
//        rt_kprintf("Dual Stepper → %lu Hz, dir1=%s, dir2=%s\n",
//                   freq,
//                   dir1 ? "CCW" : "CW",
//                   dir2 ? "CCW" : "CW");
//
//        rt_thread_mdelay(2000);
//    }
//}
//
//int main(void)
//{
//    rt_thread_t tid = rt_thread_create("dualstep",
//                                       dual_stepper_thread, RT_NULL,
//                                       1024, RT_THREAD_PRIORITY_MAX - 2, 10);
//    if (tid)
//        rt_thread_startup(tid);
//    else
//        rt_kprintf("Failed to create dual stepper thread\n");
//
//    return RT_EOK;
//}

