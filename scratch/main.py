
import re

# 初始化一个字典来存储每个用户的延迟数据
delays_by_user = {}

# 打开文件并读取
with open('scratch/log.adjustGear_2.server.out', 'r', encoding='utf-8') as file:
    for line in file:
        match = line.__contains__('UdpServer:HandleRead(): TraceDelay: RX ')
        if match:
            user_id = int(line[line.find('s ')+2:line.find(' UdpServer:HandleRead(): TraceDelay: ')])
            delay = line[line.find(' Delay: ')+9:line.find('ns',line.find(' Delay: '))]

            if user_id not in delays_by_user:
                delays_by_user[user_id] = []
            delays_by_user[user_id].append(float(delay)/1000000)
# for id in delays_by_user:
#     print(str(id)+str(delays_by_user[id]))


# 获取排序后的用户 ID 列表
sorted_user_ids = sorted(delays_by_user.keys())

# 遍历排序后的用户 ID，计算小于50的延迟值的比例，并输出统计个数
for user_id in sorted_user_ids:
    delays = delays_by_user[user_id]
    total_delays = len(delays)
    # 统计小于50的延迟值数量
    count_less_than_50 = sum(1 for delay in delays if delay < 50)
    # 计算比例
    proportion = count_less_than_50 / total_delays if total_delays > 0 else 0
    # 输出结果，包含总延迟数和小于50的延迟数
    print(f"用户 ID {user_id}: 延迟小于50的比例为 {proportion:.2%} (总数: {total_delays}, 小于50的数量: {count_less_than_50})")