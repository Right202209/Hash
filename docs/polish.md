# Hash 工具箱 UI 美化规范（v1.2 视觉打磨）

> **状态**：提案（待评审）　**适用版本**：v1.2.0 起　**读者**：设计、前端（QML）、评审
> **定位**：在 PRD 附录 A（UI 设计规范）基础上做视觉与交互打磨。本文不改变任何功能、CLI 或兼容面；GUI 视觉属于 PRD §7 中可在 minor 版本调整的部分。
> **合入规则**：每项落地后，把结论写回 PRD 附录 A，并在本文中标记「已合入」。两者冲突时，**以附录 A 为准**，直到本文对应条目合入。

---

## 0. 摘要

| 维度     | 现状（v1.1.0）                                         | 目标（v1.2）                                          |
| -------- | ------------------------------------------------------ | ----------------------------------------------------- |
| 令牌     | 只有颜色、圆角、时长；间距与字号散落在各页面           | 间距、字号、尺寸、层级、语义色全部令牌化              |
| 对比度   | 正文充足；部分组合（选中行上的次要文字）不达标         | 所有文字组合 ≥ 4.5:1，大字与图形 ≥ 3:1，附校验表      |
| 焦点     | 只有输入框有聚焦边框                                   | 所有可交互控件都有键盘焦点环（仅在键盘操作时显示）    |
| 层级     | 命令面板与页面在同一平面                               | 遮罩 + 双层描边，不用阴影也有清晰的浮层感             |
| 反馈     | Toast 只有文字；比对结果是一行计数                     | Toast 分类型；比对与完成小结用横幅 + 统计胶囊呈现     |
| 组件     | 「标签 + 值 + 复制」等模式在 4 个页面重复实现          | 新增 6 个共享 `App*` 组件，统一实现                   |
| 适配     | 未规定缩放与最小窗口下的表现                           | 100% / 125% / 150% 与 840×560 截图验收               |
| 体积     | —                                                      | **零新增 Qt 模块**，发布 zip 增幅 ≤ 1%                |

---

## 1. 硬约束（不可突破）

以下约束来自 PRD，任何美化方案都不能违反：

1. **零图片、零 SVG**：不引入 QtSvg 与 imageformats；图标仍为字符字形（NFR-PERF-6）。
2. **不新增 Qt 模块**：不使用 `QtQuick.Effects`（MultiEffect / 阴影）、`QtQuick.Shapes`（虚线、矢量）、`Qt5Compat.GraphicalEffects`。理由：体积是产品属性（PRD §5）。
3. **只用 Basic 样式**（NFR-PERF-7），所有外观通过 `App*` 控件的 `background` / `contentItem` 定制。
4. **`Theme.qml` 是唯一取色来源**；页面中禁止裸色值、自定义圆角或时长（附录 A.9）。
5. **动效极简**：只允许颜色、透明度过渡；**禁止位移、缩放、弹跳动画**（附录 A.8）。
6. **状态双通道**：颜色之外必须同时有文字或符号（NFR-UX-3）。
7. **GUI 线程不做重活**：美化不能引入逐帧的 JS 计算或大量绑定重算。
8. **文案用中文，并以 `qsTr()` 包裹**。

---

## 2. 设计令牌扩展

> 原有令牌（`canvas`…`danger`、`radius`、`cardRadius`、`fadeMs` 等）**数值不变**，只做补充。
> **同步规则澄清**：只有映射到 `QPalette` 角色的颜色需要在 `Theme.qml`、`Main.qml`、`main.cpp` 三处同步；纯 QML 令牌（如间距、语义软色）只改 `Theme.qml`。

### 2.1 间距（4 px 网格）

| 令牌     | 值  | 典型用途                                   |
| -------- | --- | ------------------------------------------ |
| `space1` | 4   | 图标与文字的间距、胶囊内边距               |
| `space2` | 8   | 同组控件间距、行内元素间距                 |
| `space3` | 12  | 卡片内边距、侧栏外边距、卡片网格间距       |
| `space4` | 16  | 区块之间的间距                             |
| `space5` | 20  | `pagePadding`（不变）                      |
| `space6` | 24  | 页面内大区块之间的间距                     |
| `space8` | 32  | 空态上下留白                               |

**规则**：新代码中的 `spacing` / `margins` / `padding` 只能使用以上令牌；旧值在页面改造时一并迁移（表单类正文 10 → `space3` 12，文本流类 6 → `space2` 8）。

### 2.2 尺寸族

| 令牌              | 值  | 用途                                   |
| ----------------- | --- | -------------------------------------- |
| `controlHeight`   | 32  | 按钮、输入框、下拉框、侧栏搜索框（原 34 → 32，与控件统一） |
| `rowHeight`       | 36  | 侧栏工具行（原 34 → 36）、命令面板行   |
| `listRowHeight`   | 40  | 文件队列行（折叠态）                   |
| `iconBlockSm`     | 24  | 命令面板行内图标块                     |
| `iconBlock`       | 36  | 概览卡片图标块（不变）                 |
| `iconBlockLg`     | 38  | `PageHeader` 图标块（不变）            |
| `statusDot`       | 8   | 状态圆点                               |
| `kvLabelWidth`    | 96  | 键值行左侧标签列宽（原文本哈希为 110） |

### 2.3 字号与字重

字号沿用现有数值，但令牌化并规定行高：

| 令牌          | 字号 | 行高 | 字重   | 用途                                       |
| ------------- | ---- | ---- | ------ | ------------------------------------------ |
| `fontCaption` | 11   | 16   | Normal | 分组标题、类别胶囊、键帽、徽标             |
| `fontSmall`   | 12   | 18   | Normal | 描述、次要信息、状态文字、计数标签         |
| `fontMono`    | 13   | 20   | Normal | 等宽文本区、摘要、路径                     |
| `fontBody`    | 15   | 22   | Normal | 界面正文（应用默认）                       |
| `fontTitle`   | 18   | 26   | Bold   | `PageHeader` 标题、进制结果                |
| `fontDisplay` | 24   | 32   | Bold   | 概览页标题                                 |

- **只用 Normal 与 Bold**：`Microsoft YaHei UI` 没有 DemiBold / Medium 字重，`font.weight: Font.DemiBold` 会被合成或回退成 Bold，不同机器上表现不一致。
- **等宽字体回退链**：`Cascadia Mono` → `Consolas` → `monospace`。部分 Windows 10 机器可能没有 Cascadia Mono（待在 Win10 镜像上确认），Consolas 是可靠的回退。实现：

  ```qml
  readonly property string monoFamily: {
      const fams = Qt.fontFamilies()
      if (fams.indexOf("Cascadia Mono") >= 0) return "Cascadia Mono"
      if (fams.indexOf("Consolas") >= 0) return "Consolas"
      return "monospace"
  }
  ```
- 等宽字体中的中文（如路径里的中文目录名）会回退到雅黑，字宽不等属预期，不做处理。

### 2.4 语义色补充

| 令牌            | 值          | 用途                                                    |
| --------------- | ----------- | ------------------------------------------------------- |
| `mutedOnAccent` | `#B8C0CF`   | 位于 `iceDeep` 选中底上的次要文字（替代 `muted`，原组合不达标） |
| `focusRing`     | = `ice`     | 键盘焦点环                                              |
| `okSoft`        | `#245FD38D` | 成功横幅底色（`ok` 约 14% 透明度，`#AARRGGBB`）         |
| `dangerSoft`    | `#24F2777A` | 失败 / 差异横幅底色、`已变化` 徽标底色                  |
| `iceSoft`       | `#337AA2FF` | 拖拽悬停时的覆盖层                                      |
| `scrim`         | `#99000000` | 命令面板遮罩（黑色 60%）                                |
| `edgeHighlight` | `#0FFFFFFF` | 浮层顶边 1 px 高光（白色约 6%），用于无阴影的层次感     |

### 2.5 圆角、层级与动效

| 令牌         | 值   | 用途                                       |
| ------------ | ---- | ------------------------------------------ |
| `radiusSm`   | 4    | 键帽、徽标、进度条（原 3 → 4，统一）       |
| `radius`     | 8    | 不变                                       |
| `cardRadius` | 12   | 不变                                       |
| `radiusPill` | 999  | 类别胶囊、统计胶囊                         |
| `fadeMs`     | 120  | 不变：hover / press 颜色过渡               |
| `overlayMs`  | 160  | 浮层（命令面板、遮罩）透明度淡入淡出       |

**层级模型（不用阴影）**：

| 层级 | 底色      | 边界                                       | 示例                 |
| ---- | --------- | ------------------------------------------ | -------------------- |
| L0   | `canvas`  | —                                          | 窗口画布             |
| L1   | `surface` | `line` 1 px                                | 侧栏、卡片、状态栏   |
| L2   | `raised`  | `line` 1 px                                | hover 行、展开区     |
| L3   | `raised`  | 外 `line` 1 px + 内顶边 `edgeHighlight` 1 px，下方叠 `scrim` | 命令面板、对话框 |
| 槽位 | `inset`   | `line` 1 px                                | 输入框、队列容器     |

### 2.6 `Theme.qml` 新增片段

```qml
// Theme.qml —— v1.2 新增令牌（原有令牌保持不变）
readonly property int space1: 4
readonly property int space2: 8
readonly property int space3: 12
readonly property int space4: 16
readonly property int space6: 24
readonly property int space8: 32

readonly property int controlHeight: 32
readonly property int rowHeight: 36
readonly property int listRowHeight: 40
readonly property int statusDot: 8
readonly property int kvLabelWidth: 96

readonly property int fontCaption: 11
readonly property int fontSmall: 12
readonly property int fontMono: 13
readonly property int fontBody: 15
readonly property int fontTitle: 18
readonly property int fontDisplay: 24

readonly property int radiusSm: 4
readonly property int radiusPill: 999
readonly property int overlayMs: 160

readonly property color mutedOnAccent: "#B8C0CF"
readonly property color focusRing: ice
readonly property color okSoft: "#245FD38D"
readonly property color dangerSoft: "#24F2777A"
readonly property color iceSoft: "#337AA2FF"
readonly property color scrim: "#99000000"
readonly property color edgeHighlight: "#0FFFFFFF"
```

---

## 3. 对比度校验

按 WCAG 2.1 相对亮度公式由令牌计算（四舍五入到 0.1），用于约束文字与背景的组合。**表中未列出的组合，使用前必须先计算。**

| 前景 \ 背景     | canvas | surface | raised | inset | iceDeep | 结论                                      |
| --------------- | ------ | ------- | ------ | ----- | ------- | ----------------------------------------- |
| `text`          | 15.6   | 14.5    | 13.2   | 12.1  | 7.4     | 全部通过                                  |
| `muted`         | 6.1    | 5.7     | 5.2    | 4.7   | **2.9** | 不得用在 `iceDeep` 上；在 `inset` 上余量很小 |
| `mutedOnAccent` | —      | —       | —      | —     | 4.9     | 选中底上的次要文字一律使用它              |
| `ice`           | 7.6    | 7.1     | 6.5    | 5.9   | 3.6     | 在 `iceDeep` 上只用于 ≥ 18 px 的字或图标  |
| `ok`            | 10.1   | 9.4     | 8.6    | 7.8   | —       | 通过                                      |
| `danger`        | 6.9    | 6.4     | 5.9    | 5.4   | —       | 通过                                      |

**图形对比（WCAG 1.4.11，≥ 3:1）**：

- `line` 对 `surface` 约 1.4:1、对 `inset` 约 1.1:1，**只能用作装饰性分隔**。可交互控件不能只靠 `line` 边框被识别，必须同时具备 `inset` 填充 + 标签或占位符；hover 与聚焦态的边框（`muted` / `ice`）满足 ≥ 3:1。
- 焦点环 `ice` 对所有表面 ≥ 5.9:1，通过。

---

## 4. 共享控件打磨

### 4.1 现有控件增强

| 控件              | 变更                                                         | 优先级 |
| ----------------- | ------------------------------------------------------------ | ------ |
| 全部可交互控件    | **键盘焦点环**：`visualFocus` 为真时，在控件外 3 px 处画 2 px `focusRing` 圆角环（圆角 = 控件圆角 + 3）；鼠标点击不显示 | P0     |
| `AppButton`       | 新增 `variant`：`primary`（= 原 `highlighted`）、`secondary`（默认）、`ghost`（透明底，用于次要操作）、`danger`（`dangerSoft` 底 + `danger` 文字，用于「清空」与运行中的「取消」）。保留 `highlighted` 作为 `primary` 的别名，保证兼容 | P0     |
| `AppTextField`    | 新增 `hasError`：为真时边框改为 `danger`，与就近显示的错误文字同时出现（双通道）；占位符统一为 `muted` | P0     |
| `AppToolButton`   | 必须提供 `tip`：用于 `ToolTip`（延迟 600 ms）与 `Accessible.name`；点击区域不小于 24×24，图标字号 14 | P0     |
| `AppProgressBar`  | 高 6、圆角 `radiusSm`；填充两端为圆角；仍只在运行时可见；**不加条纹或流光动画** | P1     |
| `AlgorithmChips`  | chip 高 28、左右内边距 `space3`；新增「全选 / 清空」两个 ghost 按钮；类别之间用 `space4` 间距与 11 px 类别标签分组（数据来自注册表的类别） | P1     |
| `PageHeader`      | 下方分隔线改为与内容区等宽（左右各缩进 `pagePadding`）；描述限 1 行，超出部分省略，完整文字放在 ToolTip 中 | P1     |
| `AppScrollBar`    | 不变                                                         | —      |

焦点环参考实现：

```qml
// AppButton.qml 片段（根元素 id: control）
background: Rectangle {
    implicitHeight: Theme.controlHeight
    radius: Theme.radius
    color: control.down ? Theme.inset
         : control.hovered ? Theme.raised : Theme.surface
    border.color: control.hovered ? Theme.muted : Theme.line
    Behavior on color { ColorAnimation { duration: Theme.fadeMs } }

    Rectangle {                         // 键盘焦点环
        anchors.fill: parent
        anchors.margins: -3
        radius: Theme.radius + 3
        color: "transparent"
        border.width: 2
        border.color: Theme.focusRing
        visible: control.visualFocus
    }
}
```

### 4.2 新增共享组件

| 组件               | 用途                                  | 规格                                                         |
| ------------------ | ------------------------------------- | ------------------------------------------------------------ |
| `AppCard`          | 替代 `GroupBox` 与手写的卡片          | `surface` 底、`line` 边框、`cardRadius`；可选 `title`（`fontSmall` `muted`，下方留 `space2`）；内边距 `space4` |
| `AppKeyValueRow`   | 「标签 + 等宽值 + 复制」的统一实现    | 标签列 `kvLabelWidth`（`ice` 等宽 12）+ 值（等宽 13，中间省略，可选中）+ `⧉`；值为空时复制按钮不可用。用于文本哈希、时间戳、进制、颜色页 |
| `AppKbd`           | 快捷键键帽                            | 高 18、`radiusSm`、`inset` 底、`line` 边框、等宽 11 `muted`；用于侧栏搜索框、状态栏、命令面板底栏 |
| `AppStatusDot`     | 状态圆点                              | `statusDot` 直径，颜色取自 `Theme.statusColor`；必须与状态文字同时出现 |
| `AppBanner`        | 结果横幅（完成小结、比对结果）        | 高 36、`radius`；`kind`：`ok` / `danger` / `info`，对应 `okSoft` / `dangerSoft` / `inset` 底 + 左侧 3 px 实色条 + 前缀符号（`✓` / `✕` / `ⓘ`）+ 文字 |
| `AppEmptyState`    | 空态                                  | 居中：40 px 字形（`muted`）+ `fontBody` 标题 + `fontSmall` `muted` 提示，上下留白 `space8` |

`AppKbd` 参考实现：

```qml
// AppKbd.qml
import QtQuick

Rectangle {
    property alias text: label.text
    implicitWidth: Math.max(18, label.implicitWidth + 2 * Theme.space1)
    implicitHeight: 18
    radius: Theme.radiusSm
    color: Theme.inset
    border.color: Theme.line

    Text {
        id: label
        anchors.centerIn: parent
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontCaption
        color: Theme.muted
    }
}
```

`AppKeyValueRow` 骨架（复制统一走 `Clipboard` 服务，见 FR-GUI-9）：

```qml
// AppKeyValueRow.qml
import QtQuick
import QtQuick.Layouts

RowLayout {
    id: row
    property string label
    property string value
    signal copyRequested(string text)
    spacing: Theme.space2

    Text {
        text: row.label
        Layout.preferredWidth: Theme.kvLabelWidth
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontSmall
        color: Theme.ice
        elide: Text.ElideRight
    }
    Text {
        text: row.value
        Layout.fillWidth: true
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontMono
        color: Theme.text
        elide: Text.ElideMiddle
    }
    AppToolButton {
        text: "⧉"
        tip: qsTr("复制")
        enabled: row.value.length > 0
        onClicked: row.copyRequested(row.value)
    }
}
```

> 值的选择与省略不能兼得：`Text` 支持省略但不能选中，`TextEdit` 可选中但不支持省略。默认用 `Text` + 复制按钮；摘要这类必须完整可见的值，改为允许换行（`wrapMode: Text.WrapAnywhere`）。

---

## 5. 外壳打磨

### 5.1 侧栏

| 项       | 规格                                                         |
| -------- | ------------------------------------------------------------ |
| 工具行   | 高 `rowHeight`；左侧图标列 20 px 宽、居中（`ice`）；名称 `fontBody` |
| 当前项   | `iceDeep` 底 + 左侧 3×18 px `ice` 指示条（`radiusSm`，垂直居中）；行内次要文字改用 `mutedOnAccent` |
| hover    | `raised` 底，`fadeMs` 过渡                                   |
| 分组标题 | `fontCaption` `muted`，字距 1，上 `space3` / 下 `space1`     |
| 搜索框   | 高 `controlHeight`；左侧放大镜字形 `⌕`（需字形验证，回退 `/`）；右侧 `AppKbd` 显示「Ctrl」「K」 |
| 品牌行   | 名称 `fontBody` Bold + 版本号 `fontCaption` `muted`（来自 `HASH_VERSION`） |

### 5.2 状态栏

- 左侧：`AppStatusDot` + 消息。**Toast 增加类型**：`Toast.show(text, kind)`，`kind` ∈ `"info"`（默认，`ice`）/ `"ok"` / `"error"`；不传 `kind` 时行为与 v1.1 一致（向后兼容）。
- 右侧：`AppKbd` 键帽组合，例如 `[Ctrl][K] 快速切换 · [Esc] 返回`，分隔点为 `muted`。
- 消息超长时中间省略，完整文本放在 ToolTip 中。

### 5.3 命令面板

| 项       | 规格                                                         |
| -------- | ------------------------------------------------------------ |
| 遮罩     | 覆盖整个窗口的 `scrim`；点击遮罩关闭；透明度淡入 `overlayMs` |
| 面板     | 宽 `min(560, 窗口宽 − 2×space6)`，距顶部 96；L3 层级（外 `line` + 内顶边 `edgeHighlight`）；`cardRadius` |
| 输入区   | 高 44，无边框，字号 `fontBody`；底部 1 px `line` 分隔       |
| 结果行   | 高 `rowHeight`，最多同时显示 8 行（超出部分滚动）；左 `iconBlockSm` 图标块 + 名称 + 描述（`fontSmall`，单行省略）+ 右侧类别胶囊 |
| 当前行   | `iceDeep` 底；描述与胶囊文字改用 `mutedOnAccent`（**不得**继续用 `muted`，2.9:1 不达标） |
| 底栏     | 高 28：`[↑][↓] 选择 · [Enter] 打开 · [Esc] 关闭`（`AppKbd`）|
| 无匹配   | `AppEmptyState` 精简版：「没有匹配的工具」+ 提示「试试 sha256、json、时间戳」 |
| 动效     | 只淡入淡出透明度，**不做**缩放或下滑                         |

### 5.4 概览页

- 卡片：`surface` 底、`line` 边框、`cardRadius`，内边距 `space4`；hover 时 `raised` 底 + `ice` 边框，图标块变为 `iceDeep`（保持现状）。
- 类别胶囊统一为 `inset` 底 + `muted` 文字，**不按类别上色**（强调色只用一处，附录 A.8）。
- 卡片支持键盘：`Tab` 聚焦时显示焦点环，`Enter` / `Space` 打开。
- 标题区下方增加一行键帽提示：`按 [Ctrl][K] 快速搜索`。

---

## 6. 页面打磨

### 6.1 文件哈希页（核心，P0）

**队列行（折叠态，高 `listRowHeight`）**

```
[●] C:\Downloads\…\ubuntu-24.04.iso   [已变化]   5.73 GiB   已完成   ✕
 8   路径（弹性，中间省略）            徽标       80 右对齐   72       24
```

- `已变化` 徽标：`dangerSoft` 底、`danger` 文字、`fontCaption`、`radiusSm`、内边距 2/6。
- 大小：等宽 `fontSmall` `muted`，统一用二进制单位（B / KiB / MiB / GiB，保留 2 位小数）。**只影响 GUI 展示**，CLI 的 `size=` 仍输出字节数。
- 行之间用 1 px `line` 分隔，hover 时 `raised` 底。
- 展开态：左侧缩进 `space6`，每个算法一行 `AppKeyValueRow`，摘要允许换行。

**拖拽反馈**

- `DropArea.containsDrag` 为真时：队列容器边框变为 2 px `ice`，叠加 `iceSoft` 覆盖层，并居中显示「松开以添加 N 个文件」（N = `drag.urls.length`）。
- 不使用虚线边框（需要 `QtQuick.Shapes`，违反约束 §1.2）。

**空态**：`AppEmptyState`，字形 `#`，标题「把文件拖到这里」，提示「或点击「添加文件」· 目录暂不支持」。

**运行与结果**

| 区块     | 规格                                                         |
| -------- | ------------------------------------------------------------ |
| 主按钮   | 空闲时为 `primary`「开始计算」；运行中为 `danger`「取消」     |
| 进度     | 进度条 + 一行 `fontSmall`：「计算中 42% · ubuntu-24.04.iso · 2/5」 |
| 完成小结 | `AppBanner`：全部成功为 `ok`；含失败或已变化为 `danger`；已取消为 `info` |
| 比对结果 | `AppBanner`（`exact` 时为 `ok`「✓ 全部匹配」，否则为 `danger`「✕ 存在差异」）+ 一行 5 个**统计胶囊**：标签 `fontSmall` `muted` + 数字 `fontBody` Bold 等宽；匹配数用 `ok`，其余四项**非零时**用 `danger`、为零时用 `muted` |
| 结果区   | 只读等宽 `NoWrap`，`inset` 底；操作行改为：左侧「复制结果」「保存结果」，右侧「粘贴对比」「清除比对」（ghost） |

**最小窗口（840×560）**：页面主体放入可滚动容器；结果文本区占用剩余高度，最小 120 px，空间不足时整页滚动，不得裁切或重叠。

### 6.2 其他页面

| 页面              | 打磨要点                                                     | 优先级 |
| ----------------- | ------------------------------------------------------------ | ------ |
| 文本哈希          | 结果列表改用 `AppKeyValueRow`；输入框右下角显示「N 字符 · M 字节（UTF-8）」`fontCaption` `muted` | P1     |
| Base64 / URL      | 输入区与输出区各放入 `AppCard`；操作行居中放在两卡之间，「输入⇄输出」使用 ghost 按钮 | P1     |
| JSON 格式化       | 出错时输入框 `hasError` 为真，错误文字显示「第 N 行：…」；成功时状态栏 Toast `ok`「已格式化」 | P1     |
| 时间戳            | 两个 `GroupBox` 改为两个 `AppCard`；结果行改用 `AppKeyValueRow`；输入框 `hasError` 联动 | P1     |
| 进制转换          | 结果区放入 `AppCard`：主结果 `fontTitle` 等宽，下方用 `AppKeyValueRow` 显示十进制 | P1     |
| 颜色转换          | 色块预览改为 48×48、`radius`、`line` 边框，下层铺 8 px 棋盘格（用两个 `Rectangle` 交替的 `Grid` 实现，零图片）以显示 Alpha；三行结果改用 `AppKeyValueRow` | P1     |
| UUID / 密码生成   | 选项区放入 `AppCard`；输出区右上角显示「共 N 条」；密码页显示字符集组成提示（如「大写·小写·数字·符号」） | P2     |

---

## 7. 图标字形审计

所有图标仍为单字符字形，但需要验证在目标字体中**确实存在**，否则会被 Windows 回退到其他字体（风格不一致，甚至显示成彩色 emoji）。

| 字形 | 码位   | 用途   | 风险                                       | 候选替代（需截图确认）       |
| ---- | ------ | ------ | ------------------------------------------ | ---------------------------- |
| `⏱`  | U+23F1 | 时间戳 | 高：可能被渲染为彩色 emoji                 | 追加 U+FE0E，或改为 `◷`（U+25F7） |
| `⚿`  | U+26BF | 密码   | 高：常见字体覆盖率低                       | `✱`（U+2731）、`**`          |
| `⧉`  | U+29C9 | 复制   | 中：Cascadia Mono / 雅黑可能都没有         | `❐`（U+2750）                |
| `◐`  | U+25D0 | 颜色   | 低                                         | —                            |
| `≡`  | U+2261 | 文本哈希 | 低                                       | —                            |
| `⌕`  | U+2315 | 搜索（新增） | 中                                   | `/`                          |
| `ⓘ`  | U+24D8 | 横幅 info（新增） | 中                              | `i`                          |

**规则**：

- 图标一律放在固定宽度的容器中并居中，防止回退字体的字宽差异造成布局抖动。
- 验收时在 **Windows 10 22H2 与 Windows 11** 上分别截图，确认每个字形都是单色并跟随 `ice` / `muted` 着色。

---

## 8. 高 DPI 与缩放

- Qt 6 在 Windows 上默认按小数缩放（125%、150%），1 px 逻辑线可能被渲染为 1.25 / 1.5 物理像素，导致发虚或粗细不一。
- 建议在 `Main.qml` 中按窗口所在屏幕计算发丝线宽，并写入 `Theme.hairline`：
  `hairline = Math.max(1, Math.floor(Screen.devicePixelRatio)) / Screen.devicePixelRatio`
  所有 `border.width: 1` 与分隔线改为使用 `Theme.hairline`。（P1，需在 125% 下截图对比效果后再决定是否推广。）
- 尺寸令牌全部为整数逻辑像素，不在 QML 中写小数尺寸。

---

## 9. 可访问性

| 项       | 要求                                                         | 优先级 |
| -------- | ------------------------------------------------------------ | ------ |
| 焦点     | 全部可交互控件可用 `Tab` 到达，焦点顺序与视觉顺序一致；显示焦点环（§4.1） | P0     |
| 读屏     | 仅有图标的按钮必须设置 `Accessible.name`（取自 `tip`）；状态圆点的 `Accessible.name` 为状态文字 | P0     |
| 对比度   | 遵守 §3 校验表                                               | P0     |
| 双通道   | 横幅带符号与文字；统计胶囊带标签；`已变化` 带文字            | P0     |
| 动效     | 不引入循环动画（可选项见 §10）                               | P0     |

---

## 10. 动效规范

| 场景                 | 动效                         | 时长        | 实现                       |
| -------------------- | ---------------------------- | ----------- | -------------------------- |
| hover / press        | 颜色过渡                     | `fadeMs`    | `ColorAnimation`           |
| 命令面板与遮罩       | 透明度 0 ↔ 1                 | `overlayMs` | `OpacityAnimator`          |
| 滚动条               | 透明度淡入淡出               | `fadeMs`    | 保持现状                   |
| 拖拽悬停覆盖层       | 透明度 0 ↔ 1                 | `fadeMs`    | `OpacityAnimator`          |
| 「计算中」状态点（**P2，可选，默认关闭**） | 透明度 1 ↔ 0.35 循环 | 700 + 700   | 组内只放 `OpacityAnimator` |

- 优先使用 `Animator` 系列：在 threaded 渲染循环下它们运行在渲染线程，批量计算时不与 GUI 线程争用；在 basic 渲染循环下仍在主线程执行，但开销可以忽略。
- **禁止**：位移、缩放、弹跳、流光或条纹进度、逐帧的 JS 动画。

---

## 11. 实施计划

| 阶段 | 范围                                                         | 产出                                   |
| ---- | ------------------------------------------------------------ | -------------------------------------- |
| 1    | 令牌（§2）、焦点环与按钮变体（§4.1）、6 个新增组件（§4.2）、字形审计（§7） | 组件可在任意页面使用；无可见回归     |
| 2    | 外壳：侧栏、状态栏 + Toast 类型、命令面板、概览（§5）        | 外壳截图组                             |
| 3    | 文件哈希页（§6.1）                                           | 核心页截图组（空态 / 拖拽 / 运行 / 完成 / 比对） |
| 4    | 其余页面（§6.2）、高 DPI 发丝线（§8）                        | 全量截图矩阵                           |
| 5    | 同步 PRD 附录 A；在本文标记「已合入」                        | 文档 PR                                |

每个阶段单独提 PR；每个 PR 只改视觉，不改控制器与业务逻辑（FR-GUI-10）。

---

## 12. 验收标准

| 编号    | 标准                                                         |
| ------- | ------------------------------------------------------------ |
| UI-AC-1 | 页面 QML 中裸色值、裸数字间距、裸字号为 0（评审时用 `grep` 检查 `#[0-9A-Fa-f]{6}`、`spacing: [0-9]`、`pixelSize: [0-9]`） |
| UI-AC-2 | §3 表中所有实际使用的文字组合 ≥ 4.5:1；`muted` 不出现在 `iceDeep` 上 |
| UI-AC-3 | 只用 `Tab` / `Shift+Tab` / `Enter` / `Esc` 即可走通 PRD 的 AC-5、AC-6、AC-7，全程可见焦点环 |
| UI-AC-4 | 截图矩阵：10 个工具页 + 概览 + 命令面板 × {默认, 空态, 错误, 运行中（文件哈希）} × {100%, 125%, 150%} × {1100×720, 840×560}，无裁切、重叠或字形缺失 |
| UI-AC-5 | `windeployqt` 部署清单与 v1.1.0 相比无新增模块或插件；发布 zip 体积增幅 ≤ 1% |
| UI-AC-6 | §7 中所有字形在 Win10 与 Win11 上均为单色，并跟随主题色    |
| UI-AC-7 | 对大于 10 GB 的文件批量计算时，窗口拖动、侧栏切换、命令面板开合无可感知卡顿（与 v1.1.0 主观对比 + 录屏附在 PR 中） |
| UI-AC-8 | CI headless 冒烟通过；PRD 全部现有 AC 不回归                 |

---

## 13. PR 评审清单

- [ ] 只使用 `Theme` 令牌，没有裸色值、裸间距、裸字号
- [ ] 新增颜色若映射到 `QPalette` 角色，已同步 `Theme.qml`、`Main.qml`、`main.cpp` 三处
- [ ] 可交互控件为 `App*` 组件，键盘焦点环可见
- [ ] 仅有图标的按钮有 `tip` 与 `Accessible.name`
- [ ] 状态同时有颜色与文字或符号
- [ ] 没有位移或缩放动画；循环动画（如有）仅使用 Animator，且已评审
- [ ] 没有新增 Qt 模块、图片或 SVG
- [ ] 附有 100% 与 150% 截图，以及 840×560 截图
- [ ] 不能在 CI 中验证的视觉项已在 PR 中写明（NFR-TEST-5）

---

## 14. 非目标

- 浅色主题、可调字号（PRD §9 未来方向，本次只为其预留令牌结构）。
- 自绘无边框标题栏（会增加窗口管理的复杂度，并与 `setColorScheme` 的暗色标题栏重复）。
- 图标字体或图标图片资源。
- 任何功能、CLI、输出格式的变更。

---

## 15. 待确认事项

| 编号  | 已确认                                                         |
| ----- | ------------------------------------------------------------ |
| UQ-1  | Windows 10 机器预装 Cascadia Mono |
| UQ-2  | §7 各字形在 Win10 / Win11 上的实际渲染结果，以及替代字形的最终选择 |
| UQ-3  | 发丝线 DPR 修正（§8）在 125% 下的实际收益推广到全部边框 |
| UQ-4  | 「计算中」状态点呼吸效果（§10）可由用户设置是否启用     |
| UQ-5  | 渲染循环类型（threaded / basic）在发布环境中的实际值，影响 Animator 否脱离 GUI 线程 |
| UQ-6  | 控件尺寸微调（搜索框 34 → 32、侧栏行 34 → 36）接受 |
