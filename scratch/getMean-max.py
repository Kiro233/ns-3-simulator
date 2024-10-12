import re
import matplotlib.pyplot as plt

ueComputationLoad=[0.0074, 0.0391, 0.066, 0.0992, 0.1352, 0.0074, 0.0074, 0.0074, 0.0074]
cloudComputationLoad=[0.1577, 0.1278, 0.0918, 0.0594, 0.0318, 0.1278, 0.0918, 0.0594, 0.0318]

def static(name):
    delays_by_user = {}
    with open(name, 'r', encoding='utf-8') as file:
        for line in file:
            match = 'UdpServer:HandleRead(): TraceDelay: RX ' in line
            if match:
                user_id = int(line[line.find('s ')+2:line.find(' UdpServer:HandleRead(): TraceDelay: ')])
                delay = line[line.find(' Delay: ')+9:line.find('ns', line.find(' Delay: '))]
                gear = int(line[line.find('ns Gear: ')+9:])

                if user_id not in delays_by_user:
                    delays_by_user[user_id] = []
                delays_by_user[user_id].append(2*float(delay)/1000000+ueComputationLoad[gear]*1000.0/3+cloudComputationLoad[gear]*1000.0/14)

    sorted_user_ids = sorted(delays_by_user.keys())
    satis_ue = 0
    total_ue = 0
    for user_id in sorted_user_ids:
        delays = delays_by_user[user_id]
        total_delays = len(delays)
        count_less_than_50 = sum(1 for delay in delays if delay < 50)
        proportion = count_less_than_50 / total_delays if total_delays > 0 else 0
        print(f"用户 ID {user_id}: 延迟小于50的比例为 {proportion:.2%} 共{total_delays}个packet")
        if proportion > 0.9:
            satis_ue += 1
        total_ue += 1
    return satis_ue/total_ue

def draw(x_values, y_values, label, color, ax):
    ax.plot(x_values, y_values, marker='o', label=label, color=color, alpha=0.5)
    ax.set_title('')
    ax.set_xlabel('User')
    ax.set_ylabel('User Satisfaction')
    ax.set_ylim(0.5, 1.1)
    ax.legend()
    ax.axhline(0.9, color='gray', linestyle='--', alpha=0.5)
    ax.grid(True)

a = [3]
b = [8,9,10,11,12,13]
c = ["1.000"]
d = ["10.00","50.00","100.0"]
e = [i for i in range(1, 22)]

# Subplot
fig, axes = plt.subplots(len(d), 1, figsize=(6, 9), sharex=True, sharey=True)

for j, dey in enumerate(d):
    
    colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k']
    for i, intl in enumerate(c):
        ue_num_all = []
        sta_result_all = []
        for gn in a:
            for ue in b:
                sta_all=0
                for random in e:
                    name = f"../test-out/Gnb3-Adjust8-fair/wds-1/random-{random}/test.server-{gn}-{ue}-{intl}-{dey}.out"
                    print(name)
                    sta_result = static(name)
                    sta_all=sta_all+sta_result
                ue_num = ue 
                print(intl)
                print(dey)
                print(ue_num)
                ue_num_all.append(ue_num)
                sta_result_all.append(sta_all/21)
        draw(ue_num_all, sta_result_all, f"interval: {intl}", colors[i], axes[j])
axes[0].set_ylabel('User Satisfaction')
plt.tight_layout()
plt.show()
