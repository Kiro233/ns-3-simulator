# Trap EXIT signal to kill background jobs
trap 'if [ -n "$(jobs -p)" ]; then kill $(jobs -p); fi' EXIT

# 参数列表
gNbNum=1
ueNumPergNbList=(14 16 18 20)
adjustInteval=0.001
adjustDelayList=(0.01 0.05 0.1)
randomStreamList=(1)
windowSizeList=(1)
randomUeList=(1 2 3 4 5 6)


# 定义函数来限制后台进程数量``
limit_jobs() {
    while [ $(jobs -p | wc -l) -ge 6 ]; do
        sleep 1
    done
}

# 创建输出目录
for randomUe in "${randomUeList[@]}"; do
    for randomStream in "${randomStreamList[@]}"; do
        for windows_size in "${windowSizeList[@]}"; do
                mkdir -p "test-out/Gnb${gNbNum}-Adjust-mobile-gear-random1-6/random-${randomUe}"

                for ueNumPergNb in "${ueNumPergNbList[@]}"; do
                    # for adjustInteval in "${adjustIntevalList[@]}"; do
                        for adjustDelay in "${adjustDelayList[@]}"; do

                            # 将adjustInteval和adjustDelay从秒转换为毫秒
                            adjustInteval_ms=$(echo "$adjustInteval * 1000" | bc)
                            adjustDelay_ms=$(echo "$adjustDelay * 1000" | bc)

                            # 生成输出文件名，单位为ms
                            outputFile="test-out/Gnb${gNbNum}-Adjust-mobile-gear-random1-6/random-${randomUe}/test.server-${gNbNum}-${ueNumPergNb}-${adjustInteval_ms}-${adjustDelay_ms}.out"

                            # 限制并行运行的后台进程数量
                            limit_jobs

                            # 执行ns3命令并在后台运行
                            NS_LOG="UdpServer=info|prefix_time|prefix_node|prefix_func" ./ns3 run scratch/adjustGear_2_fair_mobile_3gnb_UMa_gear --command-template="%s --randomUe=${randomUe} --gNbNum=${gNbNum} --ueNumPergNb=${ueNumPergNb} --adjustInteval=${adjustInteval} --adjustDelay=${adjustDelay} --randomStream=${randomStream} --windows_size=${windows_size}" > "$outputFile" 2>&1 &

                            echo "Executed with gNbNum=${gNbNum}, ueNumPergNb=${ueNumPergNb}, adjustInteval=${adjustInteval_ms}ms, adjustDelay=${adjustDelay_ms}ms, output=${outputFile}, randomStream=${randomStream}, windows_size=${windows_size},"

                            
                        done
                    # done   
                done
                            
                # wait
        done
    done
done