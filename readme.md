# 初选集

## 设计要求
+ 支持在线用户300万
+ 保留7天的历史新闻数据, 淘汰粒度按照分钟算
+ 按照注册用户18天，非注册用户3天进行淘汰, 淘汰粒度按照天算   
  _注：区分注册用户和非注册用户方法，根据用户ID小于1000,000,000为注册用户_

## 功能说明
+ 记录用户订阅信息（每次为全局更新）
+ 记录用户操作记录（包括: 点击，不喜欢； 点赞，评论，收藏和分享暂时没处理）
+ 记录新闻数据信息
+ 记录用户对信息的操作记录
+ 查询用户是否存在（被淘汰后的用户当做新用户）
+ 查询用户的浏览历史记录
+ 获取推荐初选列表

## 存储数据
+ 用户数据
  - 用户最后阅读时间
  - 阅读过的新闻
  - 推荐过的新闻
  - 不喜欢的新闻
  - 用户订阅的圈子和SRP词
+ 新闻数据
  - 用户的点击计数(注：经常变动)
  - 发布时间
  - 入库时间
  - 包含图片个数
  - 所属分类（可多个）
  - 新闻关键词
  - SRP词（可多个）
  - 所属圈子（可多个）
  - 全局推荐/部分推荐
  - 部分推荐所属SRP词
  - 部分推荐所属圈子
  - 所属地域
  - 数据类型（新闻，视频）

## 数据持久化
数据按照分层存储即：L0,L1...LN, L0为顶层, 其数据为活跃状态，可对其进行修改（增，删，改）。
L1...LN为非顶层,其数据为非活跃状态，不可对其进行修改。
1. 若对处于非活跃状态数据进行修改, 采取copy-on-write方式，将其复制到顶层后再进行修改。
2. 层级之间合并只能按照从N到1合并顺序，且在合并内存中的数据时需先合并磁盘中的数据。
3. 数据结构按照元数据加数据体方式存储，元数据包含：频繁变更数据和数据体指针（注：在copy-on-write
时可以分层进行）
4. 数据淘汰通过向L0中添加删除标记，在后续层级合并时才物理删除。


