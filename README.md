# PGAI-NT — 双模态快中子材料识别仿真系统

> Material-resolved imaging using fused 4.05 MeV fast-neutron transmission and prompt-gamma spectroscopy: a Geant4 simulation study

基于 Geant4 11.3 (MT) 的双模态仿真平台：
- **Channel A** — 4.05 MeV 快中子透射成像（塑料闪烁体 EJ200 近似 `G4_PLASTIC_SC_VINYLTOLUENE`）
- **Channel B** — HPGe 瞬发伽马能谱（event 级合并 + 可选高斯分辨率展宽）

## 构建

```bash
source /etc/profile.d/geant4.sh
cmake -S . -B build
cmake --build build -j$(nproc)
```

## 运行

### 单模式（固定 macro）
```bash
./build/NT run_empty.mac                 # 空场 I0
./build/NT run_single_PE.mac             # 单材料标定 (PE/Al/Fe/Cu/Pb/Ni)
./build/NT run_material_degeneracy.mac   # 材料退化 phantom
./build/NT run_steel_shell.mac           # 钢壳含氢 phantom
```

### 一键全流程（仿真 + 分析 + 论文图）
```bash
python3 analysis/run_batch.py --events 50000      # 正式统计量
python3 analysis/run_batch.py --skip-sim          # 仅重跑分析（复用 raw）
```

## 宏命令（/pgai/...）

| 命令 | 说明 | 默认 |
|------|------|------|
| `/pgai/source/energy` | 中子平均能量 | 4.05 MeV |
| `/pgai/source/energySpread` | 分数能量展宽(1σ) | 0.0 |
| `/pgai/source/spotSize` | 源斑/照射野 | 1 mm |
| `/pgai/source/divergence` | 角发散(1σ) | 0 deg |
| `/pgai/detector/pixelsX` `/pixelsY` | 像素数 | 128 128 |
| `/pgai/detector/scintThickness` | 闪烁体厚度 | 10 mm |
| `/pgai/hpge/smear` | HPGe 高斯展宽开关 | false |
| `/pgai/phantom/mode` | empty\|single\|degeneracy\|steel | empty |
| `/pgai/phantom/singleMaterial` | PE\|Al\|Fe\|Cu\|Pb\|Ni\|air\|water | PE |
| `/pgai/run/angle` | 投影角度(tomography) | 0 deg |

> 改变几何参数（像素/厚度/phantom mode/角度）后需 `/run/initialize` 重建几何。

## 架构

```
Channel A (透射)                     Channel B (HPGe)
FastNeutronTransmissionSD            HPGeSpectrometerSD
  └─ ImagingHit (per-step)             └─ HPGeHit (per-step)
       └─ EventAction → ntuple0              └─ EventAction 合并 → ntuple1 (event-level)
```

**关键设计 (真实探测器结构)**
- **样品台旋转 CT**：phantom 放在绕 z 轴旋转的 `SampleStage` 母体内（真实中子成像 CT 设置：样品转，源+探测器固定）。
- **透射屏 (EJ-200 风格多层结构)**：[Al 入射窗 1mm] + [塑料闪烁体灵敏层 10mm] + [Al 出射窗/反光层 1mm] + Al 框架。SD 绑灵敏闪烁体层，按局部坐标算 128×128 像素（模拟 CCD 读出像素化，无死区）。
- **HPGe (真实高纯锗多层)**：Al 杜瓦外壳(2mm) + 真空层(3mm) + Ge 死层(0.7mm, 非灵敏, Li 扩散层) + Ge 灵敏芯(SD)。γ 穿过杜瓦+真空+死层在灵敏芯沉积。同心多层直接放 world（不嵌套，避免越界）。
- **Pb 锥孔准直器**：`G4SubtractionSolid`(Pb 圆柱 Ø35mm − G4Cons 锥孔)，前端小孔 5mm 对样品（聚焦视场），后端大孔 12mm 对 HPGe（覆盖晶体）。距样品 150mm 近距提升效率。
- 所有结构参数见 `PGAIConfig.hh`。
- HPGe **event 级合并**：`EventAction::EndOfEvent` 把该事件所有 HPGe step 合成一条记录（`total_edep_keV`），可选 `FWHM(E)=√(a²+bE+cE²)` 高斯展宽。
- **无非物理 kill track**（已移除原 `fStopAndKill`）。
- 4 种 phantom：`BuildEmptyPhantom` / `BuildSingleMaterialPhantom` / `BuildMaterialDegeneracyPhantom` / `BuildSteelShellHydrogenPhantom`。

## 输出目录

```
outputs/
├── raw/          pgai_run{N}_nt_{transmission|hpge_events}_t{T}.csv  (MT 分片)
├── images/       I0/I/T/A 的 .npy/.csv
├── spectra/      HPGe 谱 + 能窗特征 json
├── library/      material_library.json
├── metrics/      accuracy_table.csv, identification_summary.json
└── figures/      论文图 fig1-fig8 + hpge_spectrum_*.png
```

### CSV 字段

**transmission** (`pgai_run{N}_nt_transmission_t{T}.csv`)：
`run_id, event_id, angle_deg, pixel_x, pixel_y, edep_keV, time_ns, particle_name, track_id, parent_id, creator_process, kinetic_energy_MeV, local_x_mm, local_y_mm, local_z_mm, is_primary_neutron, is_scattered_neutron`

**hpge_events** (`pgai_run{N}_nt_hpge_events_t{T}.csv`)：
`run_id, event_id, angle_deg, total_edep_keV, n_steps, first_gamma_energy_keV, particle_names, dominant_creator_process, detector_name`

## 论文图

| 图 | 文件 | 内容 |
|----|------|------|
| Fig 1 | `fig1_geometry.png` | 双模态几何示意 |
| Fig 2 | `fig2_source_spectrum.png` | 4.05 MeV 源能量分布（不同 spread） |
| Fig 3 | `fig3_transmission_*.png` | I0 / I / T / A=-ln(I/I0) |
| Fig 4 | `hpge_spectrum_{PE,Al,Fe,Cu,Pb,Ni,mixed}.png` | 各材料瞬发伽马谱 |
| Fig 5 | `fig5_neutron_only.png` | neutron-only 材料识别（混淆） |
| Fig 6 | `fig6_gamma_only.png` | gamma-only（无空间分辨） |
| Fig 7 | `fig7_fusion.png` | 融合识别 |
| Fig 8 | `fig8_confusion_{neutron,fusion}.png` | 混淆矩阵 |

## 已验证

- ✅ 编译链接（Geant4 11.3.2 MT，16 线程）
- ✅ 双通道真实数据输出（透射投影 + HPGe event-level 谱）
- ✅ empty/sample 归一化透射图 (T, A)
- ✅ 6 材料 HPGe 谱 + 材料响应库
- ✅ neutron-only / gamma-only / fusion 三方法 + confusion matrix + accuracy table
- ✅ 全部数据来自真实 Geant4 仿真（无 mock）

## 当前限制（需大统计量）

HPGe 瞬发伽马探测效率低（小立体角 + 准直器 + 4 MeV 中子非弹性伽马产额低），PGAI 本征要求 **≥1e6 中子/材料** 才能获得有统计意义的能窗特征。验收测试用 1.2e4 事件：

- `neutron-only acc ≈ 0–17%`：degeneracy phantom 故意用不同厚度耦合材料（PE 60mm / Al 40mm / Fe 15mm / Cu 12mm / Pb 10mm），使不同材料产生相近 4.05 MeV 透射衰减 → **这正是"材料退化"论点的直接证据**。
- HPGe 能窗 G3–G5（>2 MeV）在 1.2e4 事件下命中为 0，fusion 优势需更大统计量体现。

**正式运行建议**：`python3 analysis/run_batch.py --events 1000000`（约数小时，16 核）。

Fig 9（统计量鲁棒性 1e5–1e8 扫描）与 Fig 10（能量展宽敏感性）需多次全流程，框架见 `analysis/run_batch.py`，未在本验收跑全。
