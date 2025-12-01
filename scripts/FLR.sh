#!/bin/bash
set -eo pipefail

# 配置项
LOG_FILE="/var/log/vf_reset_batch.log"  # 日志文件路径
RESET_RETRY=1                           # 复位重试次数
SLEEP_INTERVAL=2                        # 操作间隔（秒）

# 检查是否为root权限
if [ "$(id -u)" -ne 0 ]; then
    echo "错误：请使用root权限运行脚本（sudo）" >&2
    exit 1
fi

# 初始化日志文件
init_log() {
    if [ ! -f "$LOG_FILE" ]; then
        touch "$LOG_FILE"
        chmod 644 "$LOG_FILE"
        echo "$(date +'%Y-%m-%d %H:%M:%S') - VF批量复位日志初始化" >> "$LOG_FILE"
    fi
}

# 日志记录函数
log_message() {
    local level=$1
    local message=$2
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] [$level] $message" >> "$LOG_FILE"
    echo "[$level] $message"  # 控制台同时输出
}

# 验证PCI地址格式（xxxx:xx:xx.x，功能号0-7）
validate_pci_format() {
    local pci=$1
    # 正则匹配：4位域:2位总线:2位设备.1位功能号（0-7）
    if [[ ! "$pci" =~ ^[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-7]$ ]]; then
        log_message "ERROR" "PCI地址格式错误：$pci（正确格式：xxxx:xx:xx.x，功能号必须为0-7，如0000:af:00.2）"
        return 1
    fi
    return 0
}

# 验证数量为正整数
validate_count() {
    local count=$1
    if [[ ! "$count" =~ ^[1-9][0-9]*$ ]]; then
        log_message "ERROR" "复位数量必须是正整数：$count"
        return 1
    fi
    return 0
}

# 递增PCI地址（功能号0-7循环，满7则设备号+1，功能号归0）
# 例如：0000:af:00.7 → 0000:af:01.0；0000:af:01.3 → 0000:af:01.4
increment_pci_addr() {
    local pci=$1
    # 拆分PCI地址为：域:总线:设备.功能
    # 格式示例：0000:af:00.7 → domain=0000, bus=af, device=00, func=7
    local domain=$(echo "$pci" | cut -d: -f1)
    local bus=$(echo "$pci" | cut -d: -f2)
    local device_func=$(echo "$pci" | cut -d: -f3)  # 设备.功能（如00.7）
    local device=$(echo "$device_func" | cut -d. -f1)  # 设备号（如00）
    local func=$(echo "$device_func" | cut -d. -f2)    # 功能号（如7）

    # 功能号+1，若超过7则设备号+1，功能号归0
    local new_func=$((func + 1))
    local new_device=$device

    if [ $new_func -gt 7 ]; then
        # 功能号满7，设备号+1（十六进制处理）
        new_func=0
        # 设备号转为十进制+1，再转回十六进制（保持2位，小写）
        new_device_dec=$((16#$device))  # 十六进制转十进制
        new_device_dec=$((new_device_dec + 1))
        new_device=$(printf "%02x" "$new_device_dec")  # 十进制转2位十六进制
    fi

    # 重组PCI地址（功能号保持1位，设备号保持2位）
    echo "${domain}:${bus}:${new_device}.${new_func}"
}

# 生成要复位的PCI地址列表（从起始地址开始，共count个，遵循功能号0-7规则）
generate_pci_list() {
    local start_pci=$1
    local count=$2
    local pci_list=()
    local current_pci="$start_pci"
    
    for ((i=0; i<count; i++)); do
        pci_list+=("$current_pci")
        current_pci=$(increment_pci_addr "$current_pci")
    done
    
    echo "${pci_list[@]}"
}

# 验证VF是否存在且为有效VF设备（通过PCI地址）
validate_vf_pci() {
    local pci=$1
    if [ ! -d "/sys/bus/pci/devices/$pci" ]; then
        log_message "ERROR" "VF设备不存在：$pci"
        return 1
    fi
    return 0
}

# 尝试直接复位VF（通过sysfs的reset节点）
reset_vf_direct() {
    local pci=$1
    local reset_path="/sys/bus/pci/devices/$pci/reset"
    
    if [ ! -f "$reset_path" ]; then
        log_message "INFO" "VF $pci不支持直接复位（无reset节点）"
        return 1
    fi
    
    log_message "INFO" "尝试直接复位VF $pci..."
    for ((i=1; i<=$RESET_RETRY; i++)); do
        if echo 1 > "$reset_path" 2>/dev/null; then
            log_message "INFO" "VF $pci直接复位成功"
            return 0
        else
            log_message "WARN" "VF $pci直接复位第$i次失败，重试中..."
            sleep $SLEEP_INTERVAL
        fi
    done
    
    log_message "ERROR" "VF $pci直接复位失败（已重试$RESET_RETRY次）"
    return 1
}

# 通过解绑/重新绑定驱动复位VF
reset_vf_rebind() {
    local pci=$1
    local driver_path="/sys/bus/pci/devices/$pci/driver"
    local driver_name=$(basename "$(readlink "$driver_path" 2>/dev/null)")
    
    if [ -z "$driver_name" ] || [ ! -d "/sys/bus/pci/drivers/$driver_name" ]; then
        log_message "ERROR" "VF $pci未绑定驱动，无法通过解绑复位"
        return 1
    fi
    
    log_message "INFO" "尝试通过解绑驱动复位VF $pci（驱动：$driver_name）..."
    # 解绑驱动
    if ! echo "$pci" > "/sys/bus/pci/drivers/$driver_name/unbind" 2>/dev/null; then
        log_message "ERROR" "VF $pci解绑驱动失败"
        return 1
    fi
    sleep $SLEEP_INTERVAL
    
    # 重新绑定驱动
    if ! echo "$pci" > "/sys/bus/pci/drivers/$driver_name/bind" 2>/dev/null; then
        log_message "WARN" "VF $pci重新绑定驱动失败，尝试扫描PCI总线..."
        # 扫描PCI总线尝试恢复
        echo 1 > /sys/bus/pci/rescan 2>/dev/null
        sleep $SLEEP_INTERVAL
        if ! echo "$pci" > "/sys/bus/pci/drivers/$driver_name/bind" 2>/dev/null; then
            log_message "ERROR" "VF $pci重新绑定驱动彻底失败"
            return 1
        fi
    fi
    
    log_message "INFO" "VF $pci通过解绑/绑定驱动复位成功"
    return 0
}

# 复位单个VF（仅接受PCI地址）
reset_single_vf() {
    local pci=$1
    
    # 先验证PCI格式和设备有效性
    if ! validate_pci_format "$pci"; then
        return 1
    fi
    if ! validate_vf_pci "$pci"; then
        return 1
    fi
    
    # 优先直接复位，失败则尝试解绑驱动
    if reset_vf_direct "$pci"; then
        return 0
    else
        if reset_vf_rebind "$pci"; then
            return 0
        else
            log_message "ERROR" "VF $pci 复位失败（所有方法尝试完毕）"
            return 1
        fi
    fi
}

# 主函数：解析参数并批量处理
main() {
    init_log
    
    # 检查参数数量（必须为2个：起始PCI地址、复位数量）
    if [ $# -ne 2 ]; then
        log_message "ERROR" "参数错误：请输入起始PCI地址和复位数量"
        echo "用法：$0 <起始PCI地址> <复位数量>" >&2
        echo "示例：$0 0000:af:00.2 10 （从af:00.2开始，按af:00.3→...→af:00.7→af:01.0→...规则生成10个地址）" >&2
        exit 1
    fi
    
    local start_pci=$1
    local count=$2
    
    # 验证起始PCI地址格式和数量有效性
    if ! validate_pci_format "$start_pci"; then
        exit 1
    fi
    if ! validate_count "$count"; then
        exit 1
    fi
    
    # 生成要复位的PCI地址列表
    local pci_list=($(generate_pci_list "$start_pci" "$count"))
    local total=${#pci_list[@]}
    local success=0
    local failed=0
    local failed_pcis=()
    
    log_message "INFO" "开始批量复位：起始地址=$start_pci，数量=$count，目标列表：${pci_list[*]}"
    
    # 依次复位每个VF
    for pci in "${pci_list[@]}"; do
        log_message "INFO" "=== 处理VF: $pci ==="
        if reset_single_vf "$pci"; then
            success=$((success + 1))
        else
            failed=$((failed + 1))
            failed_pcis+=("$pci")
        fi
        sleep $SLEEP_INTERVAL  # 避免操作过于密集
    done
    
    # 输出汇总结果
    log_message "INFO" "批量复位完成：成功${success}个，失败${failed}个"
    if [ $failed -gt 0 ]; then
        log_message "ERROR" "复位失败的VF列表：${failed_pcis[*]}"
    fi
    exit 0
}

# 启动主函数
main "$@"