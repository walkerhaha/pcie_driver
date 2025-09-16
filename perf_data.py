import re
import pandas as pd

test_log = './MTDMA_perf.log'

case_pattern = r'TEST_CASE\s+(perf_dma_(bare_)?(chain|single)_(ddr|pcie)_(bypass_mmu|mmu))\s'

pattern = r'''
    (H2D|D2D|D2H)\d+:\s*xfer\(([0-9a-fA-F]+):([0-9a-fA-F]+):\d+\)\s*done\s+([\d.]+)s,\s*speed\s+([\d.]+)MB/s
'''

# 初始化数据结构
transfer_data = {
    'H2D': [],
    'D2D': [],
    'D2H': []
}

try:
    with open(test_log, 'r') as file:
        log_content = file.read()

        case_match = re.search(case_pattern, log_content)
        if case_match:
            chain_type = case_match.group(3)
            mmu_type = case_match.group(5)

        matches = re.findall(pattern, log_content, re.VERBOSE | re.IGNORECASE)

        for match in matches:
            transfer_type = match[0].upper()
            sar = int(match[1], 16) if '0x' in match[1] else int(match[1])  # 支持十六进制
            dar = int(match[2], 16) if '0x' in match[2] else int(match[2])
            time = float(match[3])
            speed = float(match[4])

            transfer_data[transfer_type].append({
                'sar_offset': sar,
                'dar': dar,
                'time(s)': time,
                'speed(MB/s)': speed
            })

    # 生成 Excel 文件
    for transfer_type, data in transfer_data.items():
        if data:
            df = pd.DataFrame(data)

            df_melted = pd.melt(df, id_vars=['sar_offset', 'dar'], var_name='metric', value_name='value')
            pivot_df = df_melted.pivot_table(index='sar_offset', columns=['dar', 'metric'], values='value')

            sheet_name = f'{transfer_type}_{chain_type}_{mmu_type}' if chain_type and mmu_type else transfer_type

            # 保存为 Excel 文件
            with pd.ExcelWriter(f'{transfer_type}_output.xlsx') as writer:
                pivot_df.to_excel(writer, sheet_name=sheet_name)
            print(f"{transfer_type} 数据已保存到 {transfer_type}_output.xlsx 的 {sheet_name} 工作表中")
        else:
            print(f"未找到 {transfer_type} 类型的匹配信息")

except FileNotFoundError:
    print("错误：日志文件不存在！")
except Exception as e:
    print(f"运行时错误: {str(e)}")