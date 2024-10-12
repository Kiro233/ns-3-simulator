import re
import matplotlib.pyplot as plt
gear_table = [
    [0.0074, 0.0391, 0.066, 0.0992, 0.1352, 0.0074, 0.0074, 0.0074, 0.0074],
    [0.1577, 0.1278, 0.0918, 0.0594, 0.0318, 0.1278, 0.0918, 0.0594, 0.0318],
    [998, 799, 599, 400, 200, 799, 599, 400, 200],
]

def static(name):
    delays_by_user = {}
    gears_by_user = {}
    with open(name, 'r', encoding='utf-8') as file:
        for line in file:
            match = 'UdpServer:HandleRead(): TraceDelay: RX ' in line
            if match:

                user_id = int(line[line.find('s ')+2:line.find(' UdpServer:HandleRead(): TraceDelay: ')])
                delay = line[line.find(' Delay: ')+9:line.find('ns', line.find(' Delay: '))]
                gear = line[line.find(' Gear: ')+7:]
                if user_id not in delays_by_user:
                    delays_by_user[user_id] = []
                #if user_id not in gears_by_user:
                #    gears_by_user[user_id] = []
                gr = int(gear)
                # 计算rtt时延，前面匹配到的时延*2+对应档位的计算时延（端+云）
                delays_by_user[user_id].append(float(delay)*2.0/1000000+gear_table[0][gr]*1000.0/3+gear_table[1][gr]*1000.0/14)
                #gears_by_user[user_id].append(int(gear))

    sorted_user_ids = sorted(delays_by_user.keys())
    satis_ue = 0
    total_ue = 0
    #gear_counts = {}

    for user_id in sorted_user_ids:
        # 按用户统计
        delays = delays_by_user[user_id]
        #gears = gears_by_user[user_id]
        total_delays = len(delays)
        count_less_than_50 = sum(1 for delay in delays if delay < 50)
        proportion = count_less_than_50 / total_delays if total_delays > 0 else 0
        print(f"用户 ID {user_id}: 延迟小于50的比例为 {proportion:.2%} 共{total_delays}个packet")
        if proportion > 0.9:
            satis_ue += 1
        total_ue += 1

        # 统计Gear信息
        #gear_counts[user_id] = {}
        #for gear in gears:
        #    if gear not in gear_counts[user_id]:
        #        gear_counts[user_id][gear] = 0
        #    gear_counts[user_id][gear] += 1

    return satis_ue/total_ue

def draw(x_values, y_values, label, color, ax):
    # 画图
    ax.plot(x_values, y_values, marker='o', label=label, color=color, alpha=0.5)
    ax.set_title('User Satisfaction')
    ax.set_xlabel('User')
    ax.set_ylabel('User Satisfaction')
    ax.set_ylim(0.5, 1.1)
    ax.legend()
    ax.axhline(0.9, color='gray', linestyle='--', alpha=0.5)
    ax.grid(True)

a = [3]
b = [i for i in range(6, 11)]
c = ["1.000", "10.00", "50.00"]
d = ["10.00", "50.00", "100.0"]

# Subplot
fig, axes = plt.subplots(len(d), 1, figsize=(6, 9), sharex=True, sharey=True)

for j, intl in enumerate(c):
    colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k']
    
    #gear_counts_all = {}
    for i, dey in enumerate(d):
        ue_num_all = []
        sta_result_all = []
        for gn in a:
            for ue in b:
                name = f"test.server-{gn}-{ue}-{intl}-{dey}.out"
                ue_num = gn * ue
                sta_result = static(name)
                ue_num_all.append(ue_num)
                sta_result_all.append(sta_result)
                #if gn * ue not in gear_counts_all:
                #    gear_counts_all[gn * ue] = gear_counts
        draw(ue_num_all, sta_result_all, f"delay: {dey}",
             colors[i], axes[j])

axes[0].set_ylabel('User Satisfaction')
plt.tight_layout()
plt.show()
